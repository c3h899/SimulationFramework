#ifndef BIDIR_QUAD_TREE_H_
#define BIDIR_QUAD_TREE_H_

#include <cinttypes>
#include <iostream>
#include <memory>
#include <list>

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

		union child_ptr{    // Space-saving measure for mutually exclusive data
			return_t data;  // Collection of data iterators (Relevant Details)
			node_iter node;   // Child Node in tree
			child_ptr() : node(nullptr) { }
			child_ptr(return_t &&Data) : data(std::move(Data)) {}
			child_ptr(node_iter &&Node) : node(std::move(Node)) {}
			~child_ptr(){}; // Leave data unhandled on destruction
		};

		enum position : uint8_t {
			// 2D Values are Aliased to 3D Representation in negZ
			negX_negY = 0x01, negX_negY_negZ = negX_negY, // (Behind) Down-Left
			negX_posY = 0x02, negX_posY_negZ = negX_posY, // (Behind) Up-Left
			posX_negY = 0x04, posX_negY_negZ = posX_negY, // (Behind) Down-Right
			posX_posY = 0x08, posX_posY_negZ = posX_posY, // (Behind) Up-Right
			negX_negY_posZ = 0x10, // (Front) Down-Left
			negX_posY_posZ = 0x20, // (Front) Up-Left
			posX_negY_posZ = 0x40, // (Front) Down-Right
			posX_posY_posZ = 0x80  // (Front) Up-Right
		};

		// Used for Constructing the Head
		constexpr TreeNode(G *gen) : redux(gen->get()), 
			parent(nullptr), scale(0), is_node_ptr(0x00) // <===========
		{
			children[0][0] = std::move( child_ptr(gen->get()) );
			children[0][1] = std::move( child_ptr(gen->get()) );
			children[1][0] = std::move( child_ptr(gen->get()) );
			children[1][1] = std::move( child_ptr(gen->get()) );
		}
		// Used for Constructing subsequent Nodes
		constexpr TreeNode(G *gen, node_iter &&head, uint8_t pos) : 
			parent(std::move(head)), scale(head->scale + 1), is_node_ptr(0x00)
		{
			/*
			 * (Unofficial) Right-Handed Cartesion Order
			 * Taken from declaration of childern (Official) order.
			 * ----
			 * Assert {[0][0] : NegX NegY, [0][1] : NegX PosY
			 *         [1][0] : PosX NegY, [1][1] : PosX PosY}
			 */
			switch(pos){ // Capture the previous Data as Redux; take its place
				case position::negX_negY :
					redux.data = std::move(*head.children[0][0].data);
					*head.is_node_ptr |= pos;
					*head.children[0][0].node = this;
					break;
				case position::negX_posY :
					redux.data = std::move(*head.children[0][1].data);
					head->is_node_ptr |= pos;
					*head.children[0][1].node = this;
					break;
				case position::posX_negY :
					redux.data = std::move(*head.children[1][0].data);
					head->is_node_ptr |= pos;
					*head.children[1][0].node = this;
					break;
				case position::posX_posY :
					redux.data = std::move(*head.children[1][1].data);
					head->is_node_ptr |= pos;
					*head.children[1][1].node = this;
					break;
				default:
					redux = std::move(gen->get());
					std::cerr << "Invalid Construction of Tree Node" << std::endl; 
			}
			children[0][0] = std::move( child_ptr(gen->get()) );
			children[0][1] = std::move( child_ptr(gen->get()) );
			children[1][0] = std::move( child_ptr(gen->get()) );
			children[1][1] = std::move( child_ptr(gen->get()) );
		}
		~TreeNode() {std::cout << "Goodby, Tree" << std::endl;}
		constexpr void print_traits(){
			std::cout << "[Node]\n" << "Parent: " << (void*) parent;
			std::cout << "Relative Scale: 2^-" << scale << "\n";
			std::cout << "Reduction Node " << (void*) &redux << "\n";
			std::cout << "Up-Left: ";
			if(is_node_ptr & negX_posY){
				std::cout << "Has Child Node " << (void*) &children[0][1].node;
			} else {
				std::cout << "Has Data Elem. " << (void*) &children[0][1].data;
			}
			std::cout << "\nUp-Right: ";
			if(is_node_ptr & posX_posY){
				std::cout << "Has Child Node " << (void*) &children[1][1].node;
			} else {
				std::cout << "Has Data Elem. " << (void*) &children[1][1].data;
			}
			std::cout << "\nDown-Left: ";
			if(is_node_ptr & negX_negY){
				std::cout << "Has Child Node " << (void*) &children[0][0].node;
			} else {
				std::cout << "Has Data Elem. " << (void*) &children[0][0].data;
			}
			std::cout << "\nDown-Right: ";
			if(is_node_ptr & posX_negY){
				std::cout << "Has Child Node " << (void*) &children[1][0].node;
			} else {
				std::cout << "Has Data Elem. " << (void*) &children[1][0].data;
			}
			std::cout << "\n" << std::endl;
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
		uint8_t is_node_ptr = 0x00; // Tracks if pointer is to another node (!data)
		child_ptr children[2][2]; // Default state to nullptr
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
			if(!(*head.is_node_ptr & pos)){
				Nodes.emplace_back(TreeNode(Generator.get(),
					std::forward<node_iter>(head), pos));
				*head.is_node_ptr |= pos; // Update Flags to Reflect Change <====================
				return 0;
			} else {
				std::cerr << "Error, Node is Already Branched" << std::endl;
				return 1;
			}
		}
		constexpr child_ptr* find_me(node_iter &target){
			node_iter parent = *target.parent;
			const node_iter end = Nodes.end();
			if(parent != end){ // Avoid Dereferencing Null Pointers		
				if((*parent.is_node_ptr & position::negX_posY) &&
					(*parent.children[0][1].node == target)) { 
					return &(*parent.children[0][1]);
				} else if((*parent.is_node_ptr & position::posX_posY) &&
					(*parent.children[1][1].node == target)) { 
					return &(*parent.children[1][1]);
				} else if((*parent.is_node_ptr & position::negX_negY) &&
					(*parent.children[0][0].node == target)) { 
					return &(*parent.children[0][0]);
				} else if((*parent.is_node_ptr & position::posX_negY) &&
					(*parent.children[1][0].node == target)) { 
					return &(*parent.children[1][0]);
				} else {
					return nullptr;
				}
			} else {
				return nullptr;
			}
		}
		constexpr uint8_t delete_node(node_iter &target){
			if(!(*target.is_node_ptr)){ // Verify all child nodes are Data
				recursive_free(target);
				// Re-Write the Pointer to node_iter to be interators....
			} else {
				return 1;
			}
		}
		constexpr uint8_t prune(node_iter &target){
		/* 
		 * (!) Warning NO effort to populate most recent results to
		 * Multigrid reduction node will be made. This MUST be done prior.
		 */
			if(target->parent != nullptr){
				if(!((*target).is_node_ptr)){ // Verify all child nodes are Data
					child_ptr *temp = find_me(target);
					if(temp != nullptr){
						
						// Push the Child Node's Data to the Parent
						// Free the Child's Resources
						// Delete the Child

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
			if((*head).is_node_ptr & position::negX_posY){
				auto temp = (*head).children[0][1].node;
				if(temp != end){ recursive_free(temp); }
			} else {
				Generator->erase((*head).children[0][1].data);
			}
			if(head->is_node_ptr & position::posX_posY){ 
				auto temp = (*head).children[1][1].node;
				if(temp != end){ recursive_free(temp); }
			} else {
				Generator->erase((*head).children[1][1].data);
			}
			if(head->is_node_ptr & position::negX_negY){ 
				auto temp = (*head).children[0][0].node;
				if(temp != end){ recursive_free(temp); }
			} else {
				Generator->erase((*head).children[0][0].data);
			}
			if(head->is_node_ptr & position::posX_negY){ 
				auto temp = (*head).children[1][0].node;
				if(temp != end){ recursive_free(temp); }
			} else {
				Generator->erase((*head).children[1][0].data);
			}
		}
};


#endif // BIDIR_QUAD_TREE_H_
