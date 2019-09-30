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


	// Physics Data, Array2D, ManagedVariable --- Demonstration 
	phys_ptr_st Phys = std::make_shared<phys_t>();
/*
	node_t PN1 = Phys->get();
	node_t PN2 = Phys->get();
	Phys->erase(PN2); // (!) Manual Deletions REQUIRED (!)

	// Elementary Edit Operations
	int ii = 0; for( auto&& x : *(PN1.phi) ) { x = float(ii++);	}
	    ii = 0; for( auto&& x : *(PN1.rho) ) { x = float(ii*ii); ++ii; }

	// Print Operation
	std::cout << PN1 << std::endl;
	Phys->erase(PN1); // (!) Manual Deletions REQUIRED (!)
*/

	/*** BidirQuadTree --- Testing ***/
	phys_ptr_st temp = std::shared_ptr<phys_t>(Phys);
	tree_prt_st Tree = std::make_shared<tree_t>( std::forward<phys_ptr_st>(temp), 1.0 );

	/*** Plotting --- Testing ***/
	Tree->grow_uniformly(1.0/128.0); // Hard-coded to give non-uniform branching
	Tree->print_list();
	Tree->testing_ground();
/*
	double h = 1.0;
	plot_rect(0.0, 0.0, h/2.0);
	plot_rect(0.0, 0.5, h/2.0);
	plot_rect(0.5, 0.5, h/4.0);
	plot_rect(0.5, 0.75, h/4.0);
	plot_rect(0.75, 0.75, h/4.0);
	plot_rect(0.75, 0.5, h/4.0);
	plot_rect(0.5, 0, h/2.0);
*/
/*
	// std::cout << "plt::show() is a blocking function." << std::endl;
	Tree->draw_tree();
    plt::show();
*/
}

