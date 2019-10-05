#include <iostream>
#include <math.h>
#include <memory>
#include <string>
#include <vector>

#include "Array2D.hpp"
#include "BidirQuadTree.hpp"
#include "ManagedVariable.hpp"
#include "Physics.hpp"
//#include "PlottingRoutines.hpp"

// Local Libraries

#include "matplotlibcpp.h"

int main(void){

	// Type Noise
	typedef Array2D<float> element_t; // Element
	typedef PhysicsData<element_t> phys_t; // Physics
//	typedef typename PhysicsData<element_t>::PhysicsNode node_t; // Node
	typedef BidirQuadTree<phys_t> tree_t; // Tree
	typedef typename std::shared_ptr<BidirQuadTree<phys_t>> tree_prt_st; // Tree
	typedef typename std::shared_ptr<phys_t> phys_ptr_st; // Shared Pointer

/*
	//=== Physics Data --- Testing
	phys_ptr_st Phys = std::make_shared<phys_t>();
	auto PN1 = Phys->get();
	auto PN2 = Phys->get();
	// Elementary Edit Operations
	int ii = 0; for( auto&& x : *PN1.phi ) { x = float(ii++);	}
	    ii = 0; for( auto&& x : *PN1.rho ) { x = float(ii*ii); ++ii; }
	// Print Operation
	std::cout << PN1 << std::endl;

		ii = 0; for( auto&& x : *PN2.phi ) { x = float(2.0*ii++);	}
	    ii = 0; for( auto&& x : *PN2.rho ) { x = float(2.0*ii*ii); ++ii; }
	// Print Operation
	std::cout << PN2 << std::endl;
*/

	//=== BidirQuadTree --- Testing
	phys_ptr_st Phys = std::make_shared<phys_t>();
	tree_prt_st Tree = std::make_shared<tree_t>( std::forward<phys_ptr_st>(Phys), 1.0 );

	//=== Tree --- Testing
	Tree->grow_uniformly(1.0/128.0); // Hard-coded to give non-uniform branching
	Tree->print_list();
	//Tree->debug_find_node();
	Tree->draw_tree();
    std::cout << "plt::show() is a blocking function." << std::endl;
    plt::show();
	
	//=== Tree Iterator --- Debugging
	
	for(auto iter = Tree->begin(); iter != Tree->end(); ++iter){
		//std::cout << *iter << std::endl;
		auto pos = iter.get_position();
		auto scale = iter.get_scale();
		for(int ii = 0; ii < scale; ++ii){std::cout << "      ";}
		std::cout << "Scale: " << scale << ", Position: (";
		std::cout << std::get<0>(pos) << ", " << std::get<1>(pos) << ")" << std::endl;
	}

	// Pictures
}

