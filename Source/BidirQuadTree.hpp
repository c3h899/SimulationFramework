#ifndef BIDIR_QUAD_TREE_H_
#define BIDIR_QUAD_TREE_H_

#include <cinttypes>
#include <cmath>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>

#include "Array2D.hpp"
#include "Bit.hpp"
#include "PlottingRoutines.hpp"
#include "Physics.hpp"

#define PRINT_NODES

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

	enum position : uint8_t { // Express Position in the array contenxtually
		// 2D Values are Aliased to 3D Representation, Back
		// (!) NO NOT EDIT. Values are Used as direct array access modifies.
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
		head			 = 9,
		invalid			 = 10,
	};

	// === Constructor ===
	// Constructor for head node
	constexpr TreeNode(G *gen) : redux( std::move( gen->get() ) ), 
		parent(nullptr), scale(0), is_node(0x00), rel_pos(position::head)
	{
		for(auto &&child : children){ // Lazy Programming is the best
			child.data = std::move( gen->get() );
		}
	}
	/*
	 * Used for Constructing subsequent Nodes
	 * (!) WARNING Parent Node Must be updated externally
	 */
	constexpr TreeNode(G *gen, node_iter &head, uint8_t pos) : 
		redux( std::move(head->children[pos].data) ), parent(head),
		scale(int8_t((head->scale)+1)),is_node(0x00), rel_pos(pos)
	{
		for(auto &&child : children){ // Lazy Programming is the best
			child.data = std::move( gen->get() );
		}
	}

	// === Destructor ===
	~TreeNode() {
		/*
		 * The Union is not trivially deconstructable as it's type is not tracked (internally)
		 * Tacking it internally is wasteful, but creating a class to contain an array of flagged
		 * Unions more-or-less invalidates TreeNode. Instead, the non-trivial desturction will
		 * be handelled by the TreeNode directly.
		 */
		for(auto ii = 0; ii < 4; ++ii){
			if(!Bit::is_set(is_node,ii)) {children[ii].data.~return_t();}
		}
		//std::cout << "Goodby. --TreeNode" << std::endl;
	}

	// === Accessors ===
	constexpr return_t& get_data(uint8_t pos){ // (!) Does not test for iterator validity
		return (pos > 3) ? (&redux) : ( Bit::is_set(is_node, pos) ? 
			(&children[pos].node.redux) : (&children[pos].data) );
	}
	// === Diagnostics ===
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
	union child_iter{    // Space-saving measure for mutually exclusive data
		return_t data;  // Collection of data iterators (Relevant Details)
		node_iter node; // Child Node in tree
		child_iter() : node(nullptr) { }
		child_iter(return_t &&Data) : data(std::move(Data)) {}
		child_iter(node_iter &&Node) : node(std::move(Node)) {}
		~child_iter(){}; // Leave data unhandled on destruction
	};

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
};

template<class G>
class BidirQuadTree{
	// TODO: Thead This
	// TODO: Cached Lookup of Addresses
public:
	typedef std::shared_ptr<G> gen_t;
	typedef typename G::return_t return_t;
	typedef TreeNode<G> node_t;
	typedef typename node_t::node_iter node_iter;
	typedef typename node_t::position position;
	typedef typename node_t::child_iter child_iter;

	constexpr BidirQuadTree(gen_t &&Gen, double len) : 
		phys_length(len), Generator(std::move(Gen))
	{
		Nodes.emplace_back( std::move(Generator.get()) );
		for(auto ii = 0; ii < 4; ++ii){
			cache.emplace_back(new std::unordered_map<node_t*, find_node_t>());
		}
	}
	~BidirQuadTree(){
		if(Nodes.begin() != Nodes.end()) {
			auto nodes = Nodes.begin();
			recursive_free(nodes);
		}
		auto num_elem = Nodes.size();
		if(num_elem != 0){
			std::cerr << "(Resource Leak) QuadTree Has: ";
			std::cerr << num_elem << " Nodes at destruction." << std::endl;
		}
	}
		// ==== BEGIN (ITERATOR IMPLEMENTATION) === //
/*
	class DFSTreeIterator {
	public:
		// === Typedef and Specification ===
		typedef DFSTreeIterator self_type;
		typedef std::forward_iterator_tag iterator_category;
		typedef node_t& reference;
		// === Principle Class Interacions === //
		// Constructor
		constexpr DFSTreeIterator(node_iter& start, double X_norm = 0.0,
			double Y_norm = 0.0, bool MG = true, int8_t Pos = index_pos::multigrid_i) : 
			present_node(start), x_norm(X_norm), y_norm(Y_norm), pos(Pos), 
			include_mg(MG)
		{
//			push();
//			if(!include_mg){++this;}
		}
		// Destructor
		~DFSTreeIterator() { }; // Nothing to Do
		// Assignment Operator
		// https://en.cppreference.com/w/cpp/language/operators#Assignment_operator
		constexpr self_type& operator=(const self_type& other){
			if(this != &other){
				hist(other.hist);
				node_iter(other.node_iter);
				x_norm = other.x_norm;
				y_norm = other.y_norm;
				index(other.index);
				include_mg = other.include_mg;
			}
			return (*this);
		}
		// === Custom Functionality === //
		constexpr return_t& get_data() {
			switch(pos){ // Note: Fall-through behavior is under appreciated
				case index_pos::multigrid_i : // Multigrid Reponse
				case index_pos::end_i :
				default : // (Safe Return for unspecified cases)
					return (present_node->redux);
					break;
				case up_left_i : 
				case up_right_i : 
				case down_left_i : 
				case down_right_i :
					return present_node->children[pos].data;
					break;
			}
		}
		constexpr node_iter& get_node(){return present_node;}
		constexpr std::tuple<double, double> get_position(){
			// Retuns Lower Left Corner of Data in Normalized Coordinates
			double temp_x = x_norm;
			double temp_y = y_norm;
			switch(pos){
				case index_pos::up_left_i :
					temp_y += 0.5*exp2(-1.0*present_node->scale);
					break;
				case index_pos::up_right_i :
					temp_x += 0.5*exp2(-1.0*present_node->scale);
					temp_y += 0.5*exp2(-1.0*present_node->scale);
					break;
				case index_pos::down_left_i :
					break;
				case index_pos::down_right_i :
					temp_x += 0.5*exp2(-1.0*present_node->scale);
					break;
				default : break;
			}
			return std::make_tuple(temp_x, temp_y);
		}
		constexpr int8_t get_scale(){
			int8_t offset = (pos == index_pos::multigrid_i) ? 0 : 1;
			return int8_t(present_node->scale + offset);
		}
		// === Operators === //
		// Pre-fix Increment
		constexpr self_type& operator++(){
			bool is_data = hist.empty();
			while(!is_data){
				// Increment (or ascend deque)
				if(pos < 3) {
					++pos;
					if(Bit::is_set(present_node->is_node, pos)){ // Node
						push(); // Track Previous State; pos tracks old
						present_node = present_node->children[3].node;
						switch(pos){ // Child's implies a factor of 0.5
							case index_pos::up_left_i :
								y_norm += exp2(-1.0*present_node->scale);
								break;
							case index_pos::up_right_i :
								x_norm += exp2(-1.0*present_node->scale);
								y_norm += exp2(-1.0*present_node->scale);
								break;
							case index_pos::down_left_i :
								break;
							case index_pos::down_right_i :
								x_norm += exp2(-1.0*present_node->scale);
								break;
							default : break;
						}
						pos = index_pos::multigrid_i; // Gets Incremented
						is_data = hist.empty() ? (true) : (include_mg);
					} else { is_data = true; } // Data Element
				} else {
					pop(); // Load previous state; increment accounted for
					is_data = hist.empty(); // Test if Deque consumed
					if(is_data) {pos = index_pos::end_i;} // Comparison Flag
				}
			}
			return (*this);
		}
		// Post-fix Increment (Avoid)
		constexpr self_type operator++(int){
			// This is why one should not use post-fix increment.
			self_type tmp = *this;
			++(*this);
			return tmp;
		}
		// Dereference Opererator (Gets Data Out of Iterator)
		constexpr return_t& operator*() {
			return get_data();
		}
		// Equivalence Comparison Operator
		constexpr bool operator==(const self_type& rhs) const {
			return ((pos == rhs.pos) && (present_node == rhs.present_node));
		}
		// Negation of Equivalend Comparison Operator (Slightly Faster)
		constexpr bool operator!=(const self_type& rhs) const {
			return ((pos != rhs.pos) || (present_node != rhs.present_node));
		}
		// Enumerations
		enum index_pos : int8_t{
			multigrid_i  = -1,
			up_left_i    = position::up_left,
			up_right_i   = position::up_right,
			down_left_i  = position::down_left,
			down_right_i = position::down_right,
			end_i        = 5
		};
	private:
		// Previous Node, Position in Index, X_norm, Y_norm
		typedef std::tuple<node_iter&, int8_t, double, double> hist_t;
		std::deque<hist_t> hist;
		node_iter& present_node;
		double x_norm;
		double y_norm;
		int8_t pos;
		bool include_mg;
		// Unified Push Statement
		constexpr void push(){
			hist.push_back(std::make_tuple<&present_node, pos, x_norm, y_norm>);
		}
		// Unified Pop Statement
		constexpr void pop(){
			present_node = std::get<0>( hist.back() );
			pos          = std::get<1>( hist.back() );
			x_norm       = std::get<2>( hist.back() );
			y_norm       = std::get<3>( hist.back() );
			hist.pop_back();
		}
	};
	// ==== END (Matrix ITERATOR IMPLEMENTATION) === //
	constexpr DFSTreeIterator begin(){
		auto&& start = Nodes.begin();
		return DFSTreeIterator( start, 0.0, 0.0, true,
			DFSTreeIterator::index_pos::multigrid_i);
	}
	constexpr DFSTreeIterator end(){
		auto&& start = Nodes.begin();
		return DFSTreeIterator( start, 0.0, 0.0, true,
			DFSTreeIterator::index_pos::end_i);
	}
*/
	// Drawing Functions TODO: Move out of Class, Enumerate as Friend Functions
	void draw_tree(){
		std::lock_guard<std::mutex> lock(resource_lock);
		auto head = Nodes.begin();
		if( head != Nodes.end() ){
			recursive_draw_node(head, 0.0, 0.0, phys_length);
		}
	}
	constexpr void grow_uniformly(double h){
		auto head = Nodes.begin();
		double ratio = ceil(log2(phys_length / h));
		double cell = double(ARRAY_ELEMENT_POWER);
		double Nd = ratio - cell - 1;
		int8_t N = (Nd < 0) ? int8_t(0) : int8_t(Nd);
		if(head->scale < N){ recursive_grow(head, N); }
	}
// ================== Should be Private, but are not ========================
	void debug_find_node(){ // (!!) Yes, find_node() was miserable to setup
		auto ii = Nodes.begin(); ++ii; // Node 0 Returns itself
		for(; ii != Nodes.end(); ++ii){
			for(uint8_t jj = 0; jj < 4; ++jj){
				auto tup = find_node_sym(ii, jj, ii->rel_pos); // Wrapper Call for Ease of Parsing
				const node_iter& ptr = std::get<0>(tup);
				int reason = int(std::get<1>(tup));
				std::cout << "{Node}: " << (void*) &*ii <<" (";
				int pos = int(ii->rel_pos);
				std::cout << pos;
				if     (pos == position::up_left   ){std::cout << "-UL";}
				else if(pos == position::up_right  ){std::cout << "-UR";}
				else if(pos == position::down_left ){std::cout << "-DL";}
				else if(pos == position::down_right){std::cout << "-DR";}
				else if(pos == position::head      ){std::cout << "-HEAD";}
				else if(pos == position::multigrid ){std::cout << "-GRID";}
				else if(pos == position::invalid   ){std::cout << "-BAD ";}
				else{ std::cout << "UNSPECIFIED " << pos; }
				std::cout << ") -";
				if     (jj == direction::up     ){std::cout << "--(Up)--";}
				else if(jj == direction::down   ){std::cout << "-(Down)-";}
				else if(jj == direction::left   ){std::cout << "-(Left)-";}
				else if(jj == direction::right  ){std::cout << "-(Right)";}
				else if(jj == direction::center ){std::cout << "(Center)";}
				else if(jj == direction::null   ){std::cout << "-(Null)-";}
				else if(jj == direction::invalid){std::cout << "-[BAD]--";}
				else{ std::cout << "UNSPECIFIED " << jj; }
				std::cout << "-> {Neighbor}: " << (void*) &*ptr;
				pos = (int) ptr->rel_pos;
				std::cout << " (" << pos;
				if     (pos == position::up_left   ){std::cout << "-UL  ";}
				else if(pos == position::up_right  ){std::cout << "-UR  ";}
				else if(pos == position::down_left ){std::cout << "-DL  ";}
				else if(pos == position::down_right){std::cout << "-DR  ";}
				else if(pos == position::head      ){std::cout << "-HEAD";}
				else if(pos == position::multigrid ){std::cout << "-GRID";}
				else if(pos == position::invalid   ){std::cout << "-BAD ";}
				else{ std::cout << "UNSPECIFIED " << pos; }
				std::cout << ") : ";
				if     (reason == find_qualifier::valid_node   ){std::cout << "Valid";}
				else if(reason == find_qualifier::is_data      ){std::cout << "Data";}
				else if(reason == find_qualifier::out_of_bounds){std::cout << "Out of Bounds";}
				else if(reason == find_qualifier::test1        ){std::cout << "Test.1";}
				else if(reason == find_qualifier::test2        ){std::cout << "Test.2";}
				else{ std::cout << "UNSPECIFIED " << reason; }
				std::cout << std::endl;
			}
		}
	}

	void print_list(){
		auto ii = Nodes.begin();
		std::string predicate = "";
		recursive_print_list(ii, predicate);
	}

	private:
		typedef std::list<node_t> cont_t;
		typedef std::tuple<const  node_iter, uint8_t> find_node_t; // Response from find
		typedef std::tuple<const child_iter, uint8_t> find_data_t; // Response from find
		double phys_length; // Physical Significance : Length of a Side (Square)
		gen_t Generator;
		cont_t Nodes;
		std::vector<std::unique_ptr<std::unordered_map<node_t*, find_node_t>>> cache;
		std::mutex resource_lock; // TODO: Improve Scaling by replacing Mutex
		// Enumerations
		enum direction : uint8_t { // Express Position in the array contenxtually
			// (!) NO NOT EDIT. Values are Used as direct array access modifies.
			up				 = 0,
			down			 = 1,
			left			 = 2,
			right			 = 3,
			center			 = 4,
			null			 = 5,
			invalid			 = 6,
		};
		enum find_qualifier : uint8_t {
			valid_node    = 0,
			is_data		  = 1,
			out_of_bounds = 2, // Out of Bounds
			multigrid     = 3, // Data returned points to multigrid node
			iterp_needed  = 4, // Data is for (N-1) Refinement Level
			test1		  = 5, // Unexpected Node State
			test2		  = 6  // Unexpected Data State
		};
		// Methods
		constexpr uint8_t branch(node_iter &head, uint8_t pos){
			uint8_t ret = 1;
			std::lock_guard<std::mutex> lock(resource_lock);
			if(!Bit::is_set(head->is_node, pos)){
				Nodes.emplace_back( std::move(Generator.get()), head, pos );
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
		constexpr find_node_t find_node_sym(node_iter &Child, uint8_t dir, int8_t rel_pos){
			if(Child->scale > 0) {
				return caching_find_node(Child->parent, dir, rel_pos);
			} else { return std::make_tuple(Child, find_qualifier::out_of_bounds ); } 
		}
		constexpr find_node_t caching_find_node(node_iter& Parent, uint8_t child_dir, int8_t child_pos){
			auto iter = cache[child_dir]->find(&*Parent);
			if( iter != cache[child_dir]->end() ){
				return iter->second;
			} else {
				std::lock_guard<std::mutex> lock(resource_lock);
				auto match = find_node(Parent, child_dir, child_pos);
				cache[child_dir]->emplace(&*Parent, match);
				return match;
			}
		}
		static constexpr find_data_t find_data(node_iter& Parent, uint8_t child_dir, int8_t child_pos){
			/*
			 * Entire Problem is approached from the prespective of the parent node
			 * (Attempting to) respond to a request from it's child
			 */
			// Look-up table routing[from][to]
			const uint8_t routing[4][4] = { /** When Comparisions are just too slow **/
				{position::invalid,  position::down_left,  position::invalid,   position::up_right},
				{position::invalid,  position::down_right, position::up_left,   position::invalid},
				{position::up_left,  position::invalid,    position::invalid,   position::down_right},
				{position::up_right, position::invalid,    position::down_left, position::invalid}
			};
			const uint8_t false_direction[4] = { /** ...or coding switch statements suck. **/
				direction::down,  // Request for {Up   } mapped to Down
				direction::up,    // Request for {Down } mapped to Up
				direction::right, // Request for {Left } mapped to Right
				direction::left   // Request for {Right} mapped to Left
			};
			uint8_t pos = routing[child_pos][child_dir];
			if(pos != position::invalid){ // Trivial Match Cases
				if( Bit::is_set(Parent->is_node, pos) ) {
					return std::make_tuple(Parent->children[pos].node->redux,
						find_qualifier::multigrid); // find_qualifier::valid_node
				} else{
					return std::make_tuple(Parent->children[pos].data,
						find_qualifier::is_data); // Simplest Outcome
				}
			} else {
				if(Parent->scale > 0) {
					auto tup = find_node(Parent->parent, child_dir, Parent->rel_pos);
					auto find = std::get<0>(tup);
					auto reason = std::get<1>(tup);
					if( (reason != find_qualifier::out_of_bounds) &&
						(find != Parent->parent) ){
						// Out of Bounds Disqualifies all further consideration
						if( (find->scale) == (Parent->scale) ) {
							auto false_dir = false_direction[child_dir];
							return find_node(find, false_dir, child_pos);
						} else if( (find->scale) < (Parent->scale) ) {
							return std::make_tuple(find->redux,
								find_qualifier::interp_needed);
						} else {
							return std::make_tuple(find, find_qualifier::test2);
						}
					} else { // Find Returned Parent, Implies Data (or Out of Bounds)
						return tup;
					}
				} else {
					return std::make_tuple(Parent->redux,
						find_qualifier::out_of_bounds );
				} 
			}
		}
		static constexpr find_node_t find_node(node_iter& Parent, uint8_t child_dir, int8_t child_pos){
			/*
			 * Entire Problem is approached from the prespective of the parent node
			 * (Attempting to) respond to a request from it's child
			 */
			// Look-up table routing[from][to]
			const uint8_t routing[4][4] = { /** When Comparisions are just too slow **/
				{position::invalid,  position::down_left,  position::invalid,   position::up_right},
				{position::invalid,  position::down_right, position::up_left,   position::invalid},
				{position::up_left,  position::invalid,    position::invalid,   position::down_right},
				{position::up_right, position::invalid,    position::down_left, position::invalid}
			};
			const uint8_t false_direction[4] = { /** ...or coding switch statements suck. **/
				direction::down,  // Request for {Up   } mapped to Down
				direction::up,    // Request for {Down } mapped to Up
				direction::right, // Request for {Left } mapped to Right
				direction::left   // Request for {Right} mapped to Left
			};
			uint8_t pos = routing[child_pos][child_dir];
			if(pos != position::invalid){ // Trivial Match Cases
				if( Bit::is_set(Parent->is_node, pos) ) {
					return std::make_tuple(Parent->children[pos].node,
						find_qualifier::valid_node); // find_qualifier::valid_node
				} else{
					return std::make_tuple(Parent, find_qualifier::is_data);
				}
			} else {
				// Recusively ask parent for neighbor
				if(Parent->scale > 0){
					auto tup = find_node(Parent->parent, child_dir, Parent->rel_pos);
					auto find = std::get<0>(tup);
					auto reason = std::get<1>(tup);
					if( (reason != find_qualifier::out_of_bounds) &&
						(find != Parent->parent) ){
						// Out of Bounds Disqualifies all further consideration
						if( (find->scale) == (Parent->scale) ) {
							auto false_dir = false_direction[child_dir];
							return find_node(find, false_dir, child_pos);
						} else {
							return std::make_tuple(find, find_qualifier::test1);
						}
					} else { // Find Returned Parent, Implies Data (or Out of Bounds)
						return tup;
					}
				} else {
					return std::make_tuple(Parent, find_qualifier::out_of_bounds);
				}
			}
		}
		constexpr void recursive_grow(node_iter& head, int8_t N){
			for(uint8_t ii = 2; ii < 4; ++ii){ // Depth-first search
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
		void recursive_print_list(node_iter& head, std::string predicate){
			std::cout << predicate;			
			if(head->rel_pos == position::head){
				std::cout << "{Head}  ";
			} else {
				std::cout << "{Child} ";
			}
			std::cout << &*head << " (" << (int) head->rel_pos << ")" << std::endl;
			predicate = predicate + "   |  ";
			for(auto ii = 0; ii < 4; ++ii){
				if(Bit::is_set(head->is_node, ii)){
					recursive_print_list(head->children[ii].node, predicate);
				} else {
					std::cout << predicate << "{Data}  ";
					std::cout << (void*) &head->children[ii].data << " (";
					std::cout << (int) ii << ")" << std::endl;
				}
			}
		}
		static constexpr node_iter get_node(node_iter* base, uint8_t pos, int8_t order){
			int8_t delta = base->scale - order;
			return ( (delta < 0) && Bit::is_set(base->is_node, pos) ) ? 
				(base->children[pos].node) : (base);
		}
		constexpr uint8_t prune(node_iter& target){
			/* 
			 * (!) Warning NO effort to populate most recent results to
			 * Multigrid reduction node will be made. This MUST be done prior.
			 */
			uint8_t ret = 1;
			auto parent = target->parent;
			if( parent != Nodes.end() ){ // Can Not Prune the Head of Tree
				if( !(target.is_node) ){ // Verify all child nodes are Data
					std::lock_guard<std::mutex> lock(resource_lock);
					// Push the Child Node's Data to the Parent
					parent->children[target.rel_pos] = std::move(target.redux);
					Bit::clear(parent->is_node, target.rel_pos);
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
		constexpr void recursive_free(node_iter& head){
			// (!) Untested, is believe to avoid leaking resrouces
			std::lock_guard<std::mutex> lock(resource_lock);
			const node_iter end = Nodes.end();
			recursive_free_worker(head, end);
		}
		constexpr void recursive_free_worker(node_iter& head, const node_iter& end){
			// Free the resources tracked in the tree
			// Warning (!) NOT Thread Safe
			// USE recursive_free(node_iter& head) as entry point
			for(auto ii = 0; ii < 4; ++ii){
				if(Bit::is_set(head->is_node, ii)){
					auto temp = head->children[ii].node;
					if(temp != end){ recursive_free_worker(temp, end); }
				}
			}
			Nodes.erase(head);
		}
};

#endif // BIDIR_QUAD_TREE_H_
