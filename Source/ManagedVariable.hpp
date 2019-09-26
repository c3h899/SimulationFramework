#ifndef MANAGED_VARIABLE_H_
#define MANAGED_VARIABLE_H_

#include <iostream>
//#include <memory_resource> //TODO: LIBC Lacks polymorphic allocators
#include <mutex>
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
//		typedef std::list<T, std::pmr::polymorphic_allocator> container_t;
		typedef std::list<T> container_t;
		typedef typename std::list<T>::iterator iterator_t;
//		constexpr ManagedVariable(std::size_t blocks = 0){ // Takes number of blocks/chunk
		constexpr ManagedVariable(){ // Edited to address -Wunused
//			mr = std::pmr::unsynchronized_pool_resource{sizeof(T), blocks};
		}
		~ManagedVariable() { }
		constexpr iterator_t create_element(){
			std::lock_guard<std::mutex> lock(resource_lock);
			cont.emplace_back(T());
			return std::prev(cont.end());
		}
		constexpr iterator_t create_element(T&& source){
			std::lock_guard<std::mutex> lock(resource_lock);
			cont.emplace_back(std::forward<T>(source));
			return std::prev(cont.end());
		}
		template <class... Args> // Emplacement Constructor (In-place construction)
		constexpr iterator_t emplace_element(Args&&... args){
			std::lock_guard<std::mutex> lock(resource_lock);
			cont.emplace_back(std::forward<Args>(args)...);
			return std::prev(cont.end());
		}
		constexpr void erase(iterator_t &&element) {
			if(element != cont.end()){cont.erase(std::forward<iterator_t>(element));}
		}
		constexpr friend std::ostream& operator<<(std::ostream &stream, const ManagedVariable<T> &var){
			std::lock_guard<std::mutex> lock(var.resource_lock); // Very limited good
			auto ii = var.cont.begin();
			const auto jj = var.cont.end();
			if(ii != jj){stream << *ii; ++ii;}			
			for(; ii != jj; ++ii){stream << ";\n" << *ii;}
			return stream;
		}
	private:
		std::mutex resource_lock; // TODO: Improve Scaling by replacing Mutex
//		std::pmr::memory_resource& mr;
//		container_t cont(&mr);
		container_t cont;
};


#endif
