#ifndef PLOTTING_ROUTINES_H_
#define PLOTTING_ROUTINES_H_

#include "matplotlibcpp.h"
#include <vector>

namespace plt = matplotlibcpp;

void plot_rect(double x, double y, double h, int n){
	// Takes Corner as Bottom-Left of Square
	// Extends by H: up and to the right
	//
	// Define the boundaries of the polygon
	std::vector<double> x1, y1;
	x1.push_back(x);     y1.push_back(y);
	x1.push_back(x);     y1.push_back(y + h); 
	x1.push_back(x + h); y1.push_back(y + h);
	x1.push_back(x + h); y1.push_back(y);
	plt::fill(x1, y1, {});
	// Give the Region a Stroke
//	x1.push_back(x);     y1.push_back(y); // Return to Start
//	plt::plot(x1, y1, "k");
	// (Cheat) Dot the Individual Cells (implicitly)
//	double h_prime = (1.0/double(n))*h;
//	std::vector<double> x3, y3;
//	for(auto ii = 0; ii < n; ++ii){
//		double y_temp = y + h_prime*(0.5 + ii);
//		for(auto jj = 0; jj < n; ++jj){
//			x3.push_back(x + h_prime*(0.5 + jj));
//			y3.push_back(y_temp);
//		}
//	}
//	plt::plot(x3,y3,"+k");
}

void show() { plt::show(); }

#endif // PLOTTING_ROUTINES
