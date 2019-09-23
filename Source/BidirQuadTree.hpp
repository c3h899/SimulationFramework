#ifndef BIDIR_QUAD_TREE_H_
#define BIDIR_QUAD_TREE_H_

#include <cinttypes>
#include <iostream>
#include <memory>

class Test{
	public:
		float val = 0;
		constexpr Test(float &&new_val) : val(std::move(new_val)) { }
//		constexpr Test(float new_val) : val(new_val) { }
		constexpr Test(){ }
		constexpr Test(Test &&other) : val(std::move(other.val)) { }
		constexpr Test(const Test &other) : val(other.val) { }
		constexpr Test& operator=(Test &&rhs) {
			val = std::move(rhs.val);
			return *this;
		}
//		constexpr Test& operator=(Test &rhs){
//			val = std::move(rhs.val);
//			return *this;
//		}
		~Test(){std::cout << "Goodby, World (" << val << ")" << std::endl;}
};

template<class T>
class TreeNode;

template<class T>
class BidirQuadTree{
	public:

	private:
};

template<class T>
class TreeNode{
	public:
		constexpr TreeNode() : multigrid(T()) {}
		constexpr TreeNode(TreeNode &&rhs) : multigrid(std::move(rhs.dat)) {}
		template <class... Args> // Emplacement Constructor (In-place construction)
		TreeNode(Args&&... args) : multigrid(T(std::forward<Args>(args)...)) {} // JFM
		TreeNode& operator=(TreeNode &&rhs) {
			multigrid(std::move(rhs.dat));
			std::cout << "Assignment. Nom." << std::endl;
			return *this;
		}
		~TreeNode() {std::cout << "Goodby, Tree" << std::endl;}
		void KeepAlive() {std::cout << "I'm Alive" << std::endl;}
	private:
		friend BidirQuadTree<T>; // The tree can interact with member variables
/* ======
 * Persistent storage used for multi-grid solver.
 * Note: Suchi is not a strictly quad-tree implementation.
 * However, the (Theoretical) 4/3 Storage Penatly is (suspected) to be a
 * good performance trade-off to continuous construction and destruction.
 */
		T multigrid;
// ======
		TreeNode<T> *parent = {nullptr}; // Parents are tracked for traversal
		/* 
		 * Child nodes may be either data or additional tree notes
		 * Care to handle member access and deletion will be needed
		 */
		uint8_t is_node_ptr = 0x00; // Tracks if pointer is to another node (!data)
		enum byte: uint8_t{ // Not Strictly the std::byte from <cstddef>
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
		union child_ptr{ // TODO Syntactically Express Overnship of Child Notes
			T *data; // Pointer to data
			TreeNode<T> *node; // Child Nodes Owned by (this)
		};
		child_ptr children[2][2] = {nullptr}; // Default state to nullptr
};

#endif // BIDIR_QUAD_TREE_H_
