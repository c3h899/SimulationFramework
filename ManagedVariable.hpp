#ifndef MANAGED_VARIABLE_H_
#define MANAGED_VARIABLE_H_

#include <iostream>
//#include <memory_resource> //TODO: LIBC Lacks polymorphic allocators
#include <mutex>
#include <memory>
#include <list>
#include <utility>

/*
 * (Un)Managed Variable
 * Enabling users to make stupid decisions at breakneck speed.
 * ---
 * Presenty, this only serves to create additional resource overhead.
 * The intent of this extra layer is to provide a basis for pooled allocation.
 * Such is reported to offer considerable performance gains based on reduced
 * cache misses. Without verification in this context, no gain can be expected.
 * Further interest will be the ability to dynamically load and unload sets of
 * computation resouces on limited heterogeneous computing architectures.
 * ---
 * Variables are managed as part of a linked-list for cheap insertions
 * ---
 * If this looks like a heap within the global heap, you're right.
 */
template <class T>
class ManagedVariable{
public:
	// Constructor
	//constexpr ManagedVariable(std::size_t blocks = 0){ // Takes number of blocks/chunk
	constexpr ManagedVariable(){ // Edited to address -Wunused
		//mr = std::pmr::unsynchronized_pool_resource{sizeof(T), blocks};
	}
	~ManagedVariable() {
		auto num_elem = cont->size();
		if(num_elem != 0){
			std::cerr << "(Resource Leak) ManagedVariable Has: ";
			std::cerr << num_elem << " Elements at destruction." << std::endl;
		}
	}
	class MVar {
		// Managed Variables behave like unique_ptrs (more or less)
	public :
		typedef typename ManagedVariable<T>::iterator_t iter_t;
		constexpr MVar(){std::cout << "I'm a bastard" << std::endl;}
// MVar Gets copied, the call to free the resource happens twice
// Code segfaults.

		constexpr MVar(const MVar&) = delete; // Copy Constructor
		constexpr MVar(MVar&& other) : it(std::move(other.it)), 
			parent(other.parent){ } // Move Constructor
		constexpr MVar& operator=(const MVar& rhs) = delete; // Copy Assignment
		constexpr MVar& operator=(MVar&& rhs){ // Move Assignement Operator
			it = std::move(rhs.it);
			parent = std::move(rhs.parent);
			return *this;
		}
		~MVar(){
			if(parent != nullptr){
				std::cout << (void*) parent << std::endl;
				std::cout << parent->cont->size() << std::endl;
				if(it != parent->cont->end()) {
					std::lock_guard<std::mutex> lock(parent->resource_lock);
//					parent->cont->erase(it);
				} else {
					std::cerr << "Error, MVar expired prior to deletion" << std::endl;
				}
			}
		}
		// Operators
		constexpr T* operator->() const { return &*it; }
		constexpr T& operator*() const { return *it; }
		// Friends
		constexpr friend std::ostream& operator<<(std::ostream &stream,
			const MVar &V)
		{
			stream << *V;
			return stream;
		}
	private :
		iter_t it;
		ManagedVariable* parent = nullptr; // ONLY USED AT DELETION
		// Used by Managed Variable
		friend ManagedVariable<T>;
		constexpr MVar(iter_t &&IT, ManagedVariable* Parent) : 
			it(std::move(IT)), parent(Parent) { }
	};

	constexpr MVar create_element(){
		std::lock_guard<std::mutex> lock(resource_lock);
		cont->emplace_back(T());
		return MVar(std::prev(cont->end()), this);
	}
	constexpr MVar create_element(T&& source){
		std::lock_guard<std::mutex> lock(resource_lock);
		cont->emplace_back(std::forward<T>(source));
		return MVar(std::prev(cont->end()), this);
	}
	template <class... Args> // Emplacement Constructor (In-place construction)
	constexpr MVar emplace_element(Args&&... args){
		std::lock_guard<std::mutex> lock(resource_lock);
		cont->emplace_back(std::forward<Args>(args)...);
		return MVar(std::prev(cont->end()), this);
	}
	constexpr friend std::ostream& operator<<(std::ostream &stream, const ManagedVariable<T> &var){
		std::lock_guard<std::mutex> lock(var.resource_lock); // Very limited good
		auto ii = var.cont->begin();
		const auto jj = var.cont->end();
		if(ii != jj){stream << *ii; ++ii;}			
		for(; ii != jj; ++ii){stream << ";\n" << *ii;}
		return stream;
	}
private:
	// Friends
	friend class MVar;

	// Type Definitions
	typedef std::list<T> cont_t;
	typedef typename cont_t::iterator iterator_t;
	typedef typename std::unique_ptr<cont_t> ptr_t;
	// typedef std::list<T, std::pmr::polymorphic_allocator> container_t;

	// Resources
	std::mutex resource_lock; // TODO: Improve Scaling by replacing Mutex
	// std::pmr::memory_resource& mr;
	// std::list<T> cont(&mr);
	ptr_t cont = std::make_unique<cont_t>();
};

#endif
