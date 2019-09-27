#ifndef BIDIR_QUAD_TREE_H_
#define BIDIR_QUAD_TREE_H_

#include <cinttypes>
#include <iostream>
#include <list>
#include <memory>
#include <tuple>

#include "Physics.hpp"

/*
 * Interface Specifications (Drafted as Needed)
 * G is a resource generator. 
 * G Defines G::return_type
 * G Implements T G::get() which returns T's to populate the tree
 */
template<class G>
class BidirQuadTree;

class bit{
	/*
	 * CPP Should express intent.
	 * ---
	 * This Class provides bit manipulation of bytes (uint8_t)
	 * Ideally, this entire class is optimized away.
	 */
	public:
		typedef uint8_t byte;
		// GCC Throws -Wconversion when using any of the stock bitwise operators
		static constexpr byte pos(int position) { return flag(position); }
		static constexpr void set(byte& rhs, int pos) { rhs = rhs | flag(pos) ; }
		static constexpr void clear(byte& rhs, int pos) { rhs = rhs & nflag(pos); }
		static constexpr byte is_set(byte const& rhs, int pos) { return rhs & flag(pos); }
	private:
		static constexpr byte Pos[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
		static constexpr byte flag(int position) { return Pos[position & 0x0F]; }
		static constexpr byte nflag(int position) { return byte(~Pos[position & 0x0F]); }
};

template<class G>
class TreeNode {
	public:
		typedef typename G::return_t return_t;
		typedef typename std::list<TreeNode<G>>::iterator node_iter;
		union child_ptr{    // Space-saving measure for mutually exclusive data
			return_t data;  // Collection of data iterators (Relevant Details)
			node_iter node;   // Child Node in tree
			child_ptr() : node(nullptr) { }
			child_ptr(return_t &&Data) : data(std::move(Data)) {}
			child_ptr(node_iter &&Node) : node(std::move(Node)) {}
			~child_ptr(){}; // Leave data unhandled on destruction
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
			down_right_front = 7
			 */
		};

		// Used for Constructing the Head
		constexpr TreeNode(G *gen) : redux(gen->get()), 
			parent(nullptr), scale(0), is_node(0x00) // <=======================
		{
			for(auto &&child : children){ // Lazy Programming is the best
				child = std::move( child_ptr(gen->get()) );
			}
		}
		// Used for Constructing subsequent Nodes
		constexpr TreeNode(G *gen, node_iter &&head, uint8_t pos) : 
			parent(std::move(head)), scale(head->scale + 1), is_node(0x00)
		{
			if(pos < 4){ // Capture the previous Data as Redux; take its place
				redux.data = std::move(*head.children[pos].data);
				bit::set(*head.is_node, pos);
				*head.children[pos].node = this;
			} else {
				redux = std::move(gen->get());
				std::cerr << "Invalid Construction of Tree Node" << std::endl;
			}
			for(auto &&child : children){ // Lazy Programming is the best
				*child = std::move( child_ptr(gen->get()) );
			}
		}
		~TreeNode() {std::cout << "Goodby, Tree" << std::endl;}
		constexpr void print_traits(){
			std::cout << "[Node]\n" << "Parent: " << (void*) parent;
			std::cout << "Relative Scale: 2^-" << scale << "\n";
			std::cout << "Reduction Node " << (void*) &redux << "\n";

			for(auto ii = 0; ii < 4; ++ii){
				std::cout << "Child (" << ii << ") ";
				if(bit::is_set(is_node, ii)){
					std::cout << "is a node: " << (void*) &children[ii].node;
				} else {
					std::cout << "is data: " << (void*) &children[ii].data;
				}
			}
		}
	private:
		friend class BidirQuadTree<G>; // The tree can interact with member variables
		return_t redux; // Multigrid Reduction Node
		node_iter parent = {nullptr}; // Parents are tracked for traversal
		unsigned int scale = 0; // Degree of Reduction 
		/* 
		 * Child nodes may be either data or additional tree notes
		 * Care to handle member access and deletion will be needed
		 */
		uint8_t is_node = 0x00; // Tracks if pointer is to another node (!data)
		child_ptr children[4]; // Default state to nullptr
};

template<class G>
class BidirQuadTree{
	public:
		typedef typename G::return_t return_t;
		typedef TreeNode<G> node_t;
		typedef typename TreeNode<G>::node_iter node_iter;
		typedef std::shared_ptr<G> gen_t;
		typedef typename TreeNode<G>::position position;
		typedef typename TreeNode<G>::child_ptr child_ptr;

		constexpr BidirQuadTree(std::shared_ptr<G> &&Gen) : Generator(std::move(Gen))
			{Nodes.emplace_back(Generator.get());}
		~BidirQuadTree(){
			if(Nodes.begin() != Nodes.end()) { recursive_free(Nodes.begin()); }
		}
	private:
		typedef std::list<node_t> cont_t;		
		gen_t Generator;
		cont_t Nodes;
		constexpr uint8_t branch(node_iter &&head, uint8_t pos){
			if(!(*head.is_node & pos)){
				Nodes.emplace_back(TreeNode(Generator.get(),
					std::forward<node_iter>(head), pos));
				bit::set(*head.is_node, pos);
				return 0;
			} else {
				std::cerr << "Error, Node is Already Branched" << std::endl;
				return 1;
			}
		}
		constexpr std::tuple<child_ptr*, uint8_t> find_me(node_iter &target){
			node_iter parent = *target.parent;
			const node_iter end = Nodes.end();
			if(parent != end){
				for(auto ii = 0; ii < 4; ++ii){
					if( (bit::is_set(*parent.is_node, ii)) &&
						(*parent.children[ii].node == target) )
					{
						return std::make_tuple(&(*parent.children[ii]), ii);
					}
				}
			} else {
				return std::make_tuple<child_ptr*, uint8_t>(nullptr, 0x00);
			}
		}
		constexpr uint8_t delete_node(node_iter &target){
			if(!(*target.is_node)){ // Verify all child nodes are Data
				recursive_free(target);
			} else {
				return 1;
			}
		}
		constexpr uint8_t prune(node_iter target){
		/* 
		 * (!) Warning NO effort to populate most recent results to
		 * Multigrid reduction node will be made. This MUST be done prior.
		 */
			if( (*target).parent != Nodes.end() ){
				if( !((*target).is_node) ){ // Verify all child nodes are Data
					std::tuple<child_ptr*, uint8_t> me = find_me(target);
					child_ptr *temp  = std::get<0>(me);
					uint8_t position = std::get<1>(me);
					if(temp != nullptr){
						// Push the Child Node's Data to the Parent
						*(temp).data = std::move((*target).redux);
						*((*target).parent).is_node &= ~position;
						// Free the Child's Resources
						recursive_free(target);
						// Delete the Child
						Nodes.erase(target);
						return 0;
					} else {
						std::cerr << "Attemped to Prune() not a child." << std::endl;
						return 3;
					}
				} else {
					std::cerr << "Attemped to Prune() node with child nodes." << std::endl;
					return 2;
				}
			} else {
				std::cerr << "Attemped to Prune() node with no parent." << std::endl;
				return 1;
			}
		}
		constexpr void recursive_free(node_iter head){
			// Free the resources tracked in the tree
			// (!) When things break (segfault), start by looking here.
			const node_iter end = Nodes.end();
			for(auto ii = 0; ii < 4; ++ii){
				if(bit::is_set((*head).is_node, ii)){
					auto temp = (*head).children[ii].node;
					if(temp != end){ recursive_free(temp); }
				} else {
					Generator->erase((*head).children[ii].data);
				}
			}
			// (!) Untested, is believe to avoid leaking resrouces
			Nodes.erase(head);
		}
};

#endif // BIDIR_QUAD_TREE_H_
