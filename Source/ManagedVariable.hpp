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
 * Variable deletion occurs in O(N) worst-case performance (expensive)
 * ---
 * If this looks like a heap within the global heap, you're right.
 */
template <class T>
class ManagedVariable{
	public:
//		typedef std::list<T, std::pmr::polymorphic_allocator> container_t;
		typedef std::list<T> container_t;
		ManagedVariable(std::size_t blocks = 0){ // Takes number of blocks/chunk
//			mr = std::pmr::unsynchronized_pool_resource{sizeof(T), blocks};
		}
		T* remove_element(T *ptr){
			/* 
			 * O(N) Search for matching values
			 * Comparisons are made based on equivalence of pointers
			 * Should generalize to larger data structures.
			 * ---
			 * NOTE: Highly implementation specific, but could be O(1).
			 */
			std::lock_guard<std::mutex> lock(resource_lock);
			const auto end = cont.end();
			auto ii = cont.begin();
			for(; (ii != end) && (&*ii != ptr); ++ii){} // O(N) Search
			if(&*ii == ptr){
//				std::cout << "\n[Goodby Data]" << std::endl;
//				std::cout << "Size : (prev) " << cont.size();
				cont.erase(ii); // (!) <- Ensure the base container implements
//				std::cout << " (new) " << cont.size() << std::endl;
				return nullptr;
			} else {
				std::cerr << "Possible Resource Leak. No deletion." << std::endl;
				return ptr;
			}
		}
		T* create_element(){
			std::lock_guard<std::mutex> lock(resource_lock);
//			cont.emplace_front(T());
//			return &cont.front();
			cont.emplace_back(T());
			return &cont.back();
		}
		T* create_element(T&& source){
			std::lock_guard<std::mutex> lock(resource_lock);
//			cont.emplace_front(std::forward<T>(source));
//			return &cont.front();
			cont.emplace_back(std::forward<T>(source));
			return &cont.back();
		}
		template <class... Args> // Emplacement Constructor (In-place construction)
		T* emplace_element(Args&&... args){
			std::lock_guard<std::mutex> lock(resource_lock);
			cont.emplace_back(std::forward<Args>(args)...);
			return &cont.back();
		}
		friend std::ostream& operator<<(std::ostream &stream, const ManagedVariable<T> &var){
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
