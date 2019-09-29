#ifndef BIDIR_QUAD_TREE_H_
#define BIDIR_QUAD_TREE_H_

#include <cinttypes>
#include <cmath>
#include <iostream>
#include <list>
#include <memory>
#include <tuple>

#include "Array2D.hpp"
#include "Bit.hpp"
#include "PlottingRoutines.hpp"
#include "Physics.hpp"

/*
 * Interface Specifications (Drafted as Needed)
 * G is a resource generator. 
 * G Defines G::return_type
 * G Implements T G::get() which returns T's to populate the tree
 */
template<class G>
class BidirQuadTree;

template<class G>
class TreeNode {
	public:
		typedef typename G::return_t return_t;
		typedef typename std::list<TreeNode<G>>::iterator node_iter;
		union child_iter{    // Space-saving measure for mutually exclusive data
			return_t data;  // Collection of data iterators (Relevant Details)
			node_iter node; // Child Node in tree
			child_iter() : node(nullptr) { }
			child_iter(return_t &&Data) : data(std::move(Data)) {}
			child_iter(node_iter &&Node) : node(std::move(Node)) {}
			~child_iter(){}; // Leave data unhandled on destruction
		};

		enum position : uint8_t { // Express Position in the array contenxtually
			// 2D Values are Aliased to 3D Representation, Back
			  up_left        = 0,
			  up_right       = 1,
			down_left        = 2,
			down_right       = 3,
			/* Unused
			  up_left_back  = up_left,			
			  up_left_front  = 4,
			  up_right_back = up_right,
			  up_right_front = 5,
			down_left_back  = down_left,
			down_left_front  = 6,
			down_right_back = down_right,
			down_right_front = 7,
			 */
			multigrid        = 8,
			head			 = 9
		};

		// Used for Constructing the Head
		constexpr TreeNode(G *gen) : redux( std::move(gen->get()) ), 
			parent(nullptr), scale(0), is_node(0x00), rel_pos(position::head)
		{
			for(auto &&child : children){ // Lazy Programming is the best
				child.data = std::move( gen->get() );
			}
		}

		// Request for resource
		constexpr return_t& get_data(uint8_t pos){ // (!) Does not test for iterator validity
			return (pos > 3) ? (&redux) : ( Bit::is_set(is_node, pos) ? 
				(&children[pos].node.redux) : (&children[pos].data) );
		}

		// Destructors
		~TreeNode() {
			//std::cout << "Goodby. --TreeNode" << std::endl;
		}
		constexpr void print_traits(){
			std::cout << "[Node]\n" << "Parent: " << &parent;
			std::cout << "Relative Scale: 2^-" << scale << "\n";
			std::cout << "Reduction Node " << (void*) &redux << "\n";

			for(auto ii = 0; ii < 4; ++ii){
				std::cout << "Child (" << ii << ") ";
				if(Bit::is_set(is_node, ii)){
					std::cout << "is a node: " << (void*) &children[ii].node;
				} else {
					std::cout << "is data: " << (void*) &children[ii].data;
				}
				std::cout << "\n";
			}
		}
	private:
		friend class BidirQuadTree<G>; // The tree can interact with member variables
		return_t redux; // Multigrid Reduction Node
		node_iter parent = {nullptr}; // Parents are tracked for traversal
		int8_t scale = 0; // Degree of Reduction (Use int for robust subtraction)
		/* 
		 * Child nodes may be either data or additional tree notes
		 * Care to handle member access and deletion will be needed
		 */
		uint8_t is_node = 0x00; // Tracks if pointer is to another node (!data)
		child_iter children[4]; // Polymorphic Array of children
		uint8_t rel_pos = 0x00; // Node's Position Relative to it's Parent's Collection
		/*
		 * Used for Constructing subsequent Nodes
		 * (!) WARNING Parent Node Must be updated externally
		 */
		constexpr TreeNode(G *gen, node_iter &head, uint8_t pos) : 
			redux( std::move(head->children[pos].data) ), parent(head),
			scale(int8_t((head->scale)+1)),is_node(0x00), rel_pos(position::head)
		{
			
			for(auto &&child : children){ // Lazy Programming is the best
				child.data = std::move(gen->get());
			}
		}
};

template<class G>
class BidirQuadTree{
	public:
		typedef typename G::return_t return_t;
		typedef TreeNode<G> node_t;
		typedef typename TreeNode<G>::node_iter node_iter;
		typedef std::shared_ptr<G> gen_t;
		typedef typename TreeNode<G>::position position;
		typedef typename TreeNode<G>::child_iter child_iter;

		constexpr BidirQuadTree(std::shared_ptr<G> &&Gen, double len) : 
			phys_length(len), Generator(std::move(Gen))
		{
			Nodes.emplace_back(Generator.get());
		}
		~BidirQuadTree(){
			if(Nodes.begin() != Nodes.end()) {
				recursive_free(Nodes.begin());
			}
			auto num_elem = Nodes.size();
			if(num_elem != 0){
				std::cerr << "(Resource Leak) QuadTree Has: ";
				std::cerr << num_elem << " Nodes at destruction." << std::endl;
			}
		}
		// Drawing Functions TODO: Move out of Class, Enumerate as Friend Functions
		void draw_tree(){
			std::lock_guard<std::mutex> lock(resource_lock);
			auto head = Nodes.begin();
			if( head != Nodes.end() ){
				recursive_draw_node(head, 0.0, 0.0, phys_length);
			}
		}
// ===========================
		constexpr void grow_until(double h){
			auto head = Nodes.begin();
			double ratio = ceil(log2(phys_length / h));
			double cell = double(ARRAY_ELEMENT_POWER);
			double Nd = ratio - cell - 1;
			int8_t N = (Nd < 0) ? int8_t(0) : int8_t(Nd);
//			std::cout << "ratio: " << ratio << " cell: " << cell;
//			std::cout << " N: " << (int) N << std::endl;
			if(head->scale < N){ recursive_grow(head, N); }
		}
// ===========================
	private:
		typedef std::list<node_t> cont_t;
		double phys_length; // Physical Significance : Length of a Side (Square)
		gen_t Generator;
		cont_t Nodes;
		std::mutex resource_lock; // TODO: Improve Scaling by replacing Mutex
		constexpr uint8_t branch(node_iter &head, uint8_t pos){
			uint8_t ret = 1;
			std::lock_guard<std::mutex> lock(resource_lock);
			if(!Bit::is_set(head->is_node, pos)){
				Nodes.emplace_back( TreeNode(Generator.get(), head, pos) );
				Bit::set( head->is_node, pos );
				head->children[pos].node = std::prev(Nodes.end());
				ret = 0;
			} else {
				std::cerr << "Error, Node is Already Branched" << std::endl;
				ret = 1;
			}
			return ret;
		}
		constexpr uint8_t delete_node(node_iter &target){
			uint8_t ret = 1;
			if( !(target->is_node) ){ // Verify all child nodes are Data
				recursive_free(target);
				ret = 0;
			} else {
				ret = 1;
			}
			return ret;
		}

		constexpr void recursive_grow(node_iter& head, int8_t N){
			for(uint8_t ii = 0; ii < 4; ++ii){ // Depth-first search
				if( Bit::is_set(head->is_node, ii) ){
					std::cerr << "Attempted to Grow in Presence of Nodes" << std::endl;				
				} else {
					// Re-ordered in response to GCC warning:
					// -Wconversion promoted ~Unsigned is always non-zero
					branch(head, ii);
					auto child = head->children[ii].node;
					if(child->scale < N) {
						recursive_grow(child, N);
					}
				}
			}
		}

		static constexpr node_iter get_node(node_iter base, uint8_t pos, int8_t order){
			int8_t delta = base->scale - order;
			return ( (delta < 0) && Bit::is_set(base->is_node, pos) ) ? 
				(base->children[pos].node) : (base);
		}

		constexpr uint8_t prune(node_iter target){
		/* 
		 * (!) Warning NO effort to populate most recent results to
		 * Multigrid reduction node will be made. This MUST be done prior.
		 */
			uint8_t ret = 1;
			auto parent = target->parent;
			if( parent != Nodes.end() ){ // Can Not Prune the Head of Tree
				if( !(target->is_node) ){ // Verify all child nodes are Data
					std::lock_guard<std::mutex> lock(resource_lock);
					// Push the Child Node's Data to the Parent
					parent->children[target->rel_pos] = std::move(target->redux);
					Bit::clear(parent->is_node, target->rel_pos);
					// Free the Child's Resources
					for(auto ii = 0; ii < 4; ++ii){
						Generator->erase(target->children[ii].data);
					}
					// (!) Untested, is believe to avoid leaking resrouces
					Nodes.erase(target);
					ret = 0;
				} else {
					std::cerr << "Attemped to Prune() node with child nodes." << std::endl;
					ret = 2;
				}
			} else {
				std::cerr << "Attemped to Prune() node with no parent." << std::endl;
				ret = 1;
			}
			return ret;
		}
		// Drawing Functions TODO: Move out of Class, Enumerate as Friend Functions
		void recursive_draw_node (node_iter &node, double x, double y, double h){
//			node->print_traits();
			double h_prime = 0.5*h;
			// Down Left
			if( Bit::is_set(node->is_node, position::down_left) ){
				auto next_node = node->children[position::down_left].node;
				recursive_draw_node(next_node, x, y, h_prime);
			} else { plot_rect(x, y, h_prime, ARRAY_ELEMENT_LENGTH); }
			// Up Left	
			double temp_y = y + h_prime;
			if( Bit::is_set(node->is_node, position::up_left) ){
				auto next_node = node->children[position::up_left].node;
				recursive_draw_node(next_node, x, temp_y, h_prime);
			} else { plot_rect(x, temp_y, h_prime, ARRAY_ELEMENT_LENGTH); }
			// Up Right
			double temp_x = x + h_prime;
			if( Bit::is_set(node->is_node, position::up_right) ){
				auto next_node = node->children[position::up_right].node;
				recursive_draw_node(next_node, temp_x, temp_y, h_prime);
			} else { plot_rect(temp_x, temp_y, h_prime, ARRAY_ELEMENT_LENGTH); }
			// Down Right
			if( Bit::is_set(node->is_node, position::down_right) ){
				auto next_node = node->children[position::down_right].node;
				recursive_draw_node(next_node, temp_x, y, h_prime);
			} else { plot_rect(temp_x, y, h_prime, ARRAY_ELEMENT_LENGTH); }
		}

		constexpr void recursive_free(node_iter head){
			// Free the resources tracked in the tree
			// (!) When things break (segfault), start by looking here.
			const node_iter end = Nodes.end();
			for(auto ii = 0; ii < 4; ++ii){
				if(Bit::is_set(head->is_node, ii)){
					auto temp = head->children[ii].node;
					if(temp != end){ recursive_free(temp); }
				} else {
					std::lock_guard<std::mutex> lock(resource_lock);
					Generator->erase(head->children[ii].data);
				}
			}
			// (!) Untested, is believe to avoid leaking resrouces
			std::lock_guard<std::mutex> lock(resource_lock);
			Generator->erase(head->redux); // Don't forget the multigrid node
			Nodes.erase(head);
		}
};

#endif // BIDIR_QUAD_TREE_H_
