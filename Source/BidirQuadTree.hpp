#ifndef BIDIR_QUAD_TREE_H_
#define BIDIR_QUAD_TREE_H_

#include <cinttypes>
#include <iostream>
#include <memory>
#include <vector>

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
		typedef TreeNode<G> node_t;

		union child_ptr{    // Space-saving measure for mutually exclusive data
			return_t data;  // Collection of data iterators (Relevant Details)
			node_t *node;   // Child Node in tree
			child_ptr() : node(nullptr) { }
			child_ptr(return_t &&Data) : data(std::move(Data)) {}
			child_ptr(node_t *Node) : node(Node) {}
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
		constexpr TreeNode(G *gen) : redux(gen->get()), scale(0), is_node_ptr(0x00)
		{
			children[0][0] = std::move( child_ptr(gen->get()) );
			children[0][1] = std::move( child_ptr(gen->get()) );
			children[1][0] = std::move( child_ptr(gen->get()) );
			children[1][1] = std::move( child_ptr(gen->get()) );
		}
		// Used for Constructing subsequent Nodes
		constexpr TreeNode(G *gen, node_t *head, uint8_t pos) : 
			parent(head), scale(head->scale + 1), is_node_ptr(0x00)
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
					redux.data = std::move(head->children[0][0].data);
					head->is_node_ptr |= pos;
					head->children[0][0].node = this;
					break;
				case position::negX_posY :
					redux.data = std::move(head->children[0][1].data);
					head->is_node_ptr |= pos;
					head->children[0][1].node = this;
					break;
				case position::posX_negY :
					redux.data = std::move(head->children[1][0].data);
					head->is_node_ptr |= pos;
					head->children[1][0].node = this;
					break;
				case position::posX_posY :
					redux.data = std::move(head->children[1][1].data);
					head->is_node_ptr |= pos;
					head->children[1][1].node = this;
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
				std::cout << "Has Child Node " << (void*) children[0][1].node;
			} else {
				std::cout << "Has Data Elem. " << (void*) &children[0][1].data;
			}
			std::cout << "\nUp-Right: ";
			if(is_node_ptr & posX_posY){
				std::cout << "Has Child Node " << (void*) children[1][1].node;
			} else {
				std::cout << "Has Data Elem. " << (void*) &children[1][1].data;
			}
			std::cout << "\nDown-Left: ";
			if(is_node_ptr & negX_negY){
				std::cout << "Has Child Node " << (void*) children[0][0].node;
			} else {
				std::cout << "Has Data Elem. " << (void*) &children[0][0].data;
			}
			std::cout << "\nDown-Right: ";
			if(is_node_ptr & posX_negY){
				std::cout << "Has Child Node " << (void*) children[1][0].node;
			} else {
				std::cout << "Has Data Elem. " << (void*) &children[1][0].data;
			}
			std::cout << "\n" << std::endl;
		}
	private:
		friend class BidirQuadTree<G>; // The tree can interact with member variables
		return_t redux; // Multigrid Reduction Node
		node_t *parent = {nullptr}; // Parents are tracked for traversal
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
		typedef std::shared_ptr<G> gen_t;
		typedef typename TreeNode<G>::position position;

		constexpr BidirQuadTree(std::shared_ptr<G> &&Gen) : Generator(std::move(Gen))
			{Nodes.emplace_back(Generator.get());}
		~BidirQuadTree(){
			if(Nodes.begin() != Nodes.end()) { free_nodes(&Nodes[0]); }
		}
/*
		constexpr uint8_t branch_node(node_t *node, uint8_t position) {
			if( !((node->is_node_prt) & position) ){ // Test for existing branch
				

				*node |= position; // Set the branch flag				
			} else {
				return 0xFF; // Invalid Request (Already Branched)
			}
		}
		constexpr uint8_t prune_node(node_t *node) {
			if(node->is_node_ptr)

		}
*/
	private:
		typedef std::vector<node_t> cont_t;		
		gen_t Generator;
		cont_t Nodes;
		constexpr void branch(node_t *head, uint8_t pos){
			if(~(head->is_node_ptr & pos)){
				Nodes.emplace_back(TreeNode(Generator.get(), head, pos));
			} else {
				std::cerr << "Error, Node is Already Branched" << std::endl;
			}
		}
		constexpr void free_nodes(node_t *head){
			// Free the resources tracked in the tree
			// (!) When things break (segfault), start by looking here.
			if(head->is_node_ptr & position::negX_posY){
				auto temp = head->children[0][1].node;
				if(temp != nullptr){ free_nodes(temp); }
			} else {
				Generator->erase(head->children[0][1].data);
			}
			if(head->is_node_ptr & position::posX_posY){ 
				auto temp = head->children[1][1].node;
				if(temp != nullptr){ free_nodes(temp); }
			} else {
				Generator->erase(head->children[1][1].data);
			}
			if(head->is_node_ptr & position::negX_negY){ 
				auto temp = head->children[0][0].node;
				if(temp != nullptr){ free_nodes(temp); }
			} else {
				Generator->erase(head->children[0][0].data);
			}
			if(head->is_node_ptr & position::posX_negY){ 
				auto temp = head->children[1][0].node;
				if(temp != nullptr){ free_nodes(temp); }
			} else {
				Generator->erase(head->children[1][0].data);
			}
		}
};


#endif // BIDIR_QUAD_TREE_H_
