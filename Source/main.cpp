#include <iostream>
#include <math.h>
#include <string>

#include "Array2D.hpp"
#include "BidirQuadTree.hpp"
#include "ManagedVariable.hpp"
#include "Physics.hpp"

int main(void){

	// Base Type
	typedef Array2D<float> element_t; // Element
	// Derived Types
	typedef PhysicsData<element_t> phys_t; // Physics
	typedef typename PhysicsData<element_t>::PhysicsNode node_t; // Node

	phys_t phys;
	node_t PN1 = phys.get_node();
	node_t PN2 = phys.get_node();
	phys.erase(std::forward<node_t>(PN2)); // (!) Manual Deletions REQUIRED (!)

	std::cout << PN1 << std::endl;

	int ii = 0;
//	for(auto iter = (*(PN1.phi)).begin(); iter != (*(PN1.phi)).end(); ++iter){
//		*iter = float(ii++);
//	}

	for( auto&& x : *(PN1.phi) ) { x = float(ii++);	}

	ii = 0;
	for( auto&& x : *(PN1.rho) ) { x = float(ii*ii); ++ii; }

	std::cout << PN1 << std::endl;

	phys.erase(std::forward<node_t>(PN1)); // (!) Manual Deletions REQUIRED (!)
	

	// === Variables are "Falling Out of Scope" Ahead of Expectation" ===

//	TreeNode<Test> Leaf(float(3.14));

}
