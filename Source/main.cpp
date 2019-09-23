#include <iostream>
#include <math.h>
#include <string>

#include <vector>

#include "BidirQuadTree.hpp"
#include "Physics.hpp"

int main(void){


	Physics<float> phys;
	auto PN1 = phys.get_node();
	auto PN2 = phys.get_node();

	int ii = 0;
	for(auto iter = PN1.phi->begin(); iter != PN1.phi->end(); ++iter){
		*iter = float(ii++);
	}
	ii = 0;
	for(auto iter = PN1.rho->begin(); iter != PN1.rho->end(); ++iter){
		*iter = float(ii*ii); ++ii;
	}

	std::cout << PN1 << std::endl;

	phys.free_vars(PN1);
	phys.free_vars(PN2);


//	Test(float(2.718));
	Test test2(Test(float(2.718)));
	

	TreeNode<Test> Leaf(float(3.14));
//	std::vector<Test> V;
//	V.emplace_back(float(3.14)));

//	TreeNode<Test> TN();
//	Leaf.KeepAlive();

}
