#ifndef ARRAY_2D_H_
#define ARRAY_2D_H_

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>

// Defines
#define ARRAY_ELEMENT_POWER 4 // MUST be aligned to a power of two
#define ARRAY_ELEMENT_LENGTH (1 << ARRAY_ELEMENT_POWER)
#define ARRAY_TOTAL_WIDTH (ARRAY_ELEMENT_LENGTH + 2)

// Warning NOT THREAD SAFE
template <class T>
class Array2D {
	public:
		constexpr T get_element(const int_fast8_t row, const int_fast8_t col) const {
			return dat[query_bounds(row)][query_bounds(col)];
		}
		constexpr void set_element(const int_fast8_t row, const int_fast8_t col, T val){
			dat[query_bounds(row)][query_bounds(col)] = std::move(val);
		}
	/* ==== BEGIN (Matrix ITERATOR IMPLEMENTATION) === */
	class MatIterator {
		/* By virtue of the O(const) access time, this iterator is a candidate for 
		 * the random access iterator tag
		 * Iterator Exhibits copy on write semantics
		 */
		public:
			// === Typedef and Specification ===
			typedef MatIterator self_type;
			typedef std::bidirectional_iterator_tag iterator_category;
			typedef T value_type ;
			typedef uint_fast16_t difference_type;
			typedef uint_fast16_t size_type;
			typedef T* pointer;
			typedef T& reference;
			// Access to neighbor elements
			constexpr T prev_x() const { return *(present - 1); }
			constexpr T next_x() const { return *(present + 1); }
			constexpr T prev_y() const { return *(present - ARRAY_TOTAL_WIDTH); }
			constexpr T next_y() const { return *(present + ARRAY_TOTAL_WIDTH); }
			// Update position
			constexpr void index_from(const MatIterator& other){ pos = other.pos; update_ptr(); }
			// === Principle Operators ===
			// Copy Constructor
			constexpr MatIterator(const self_type& other) : ptr(other.ptr), 
				present(other.present), pos(other.pos) { }
			constexpr MatIterator(pointer pZero, size_type Pos = 0) : 
				ptr(std::move(pZero)), pos(std::move(Pos)) { update_ptr(); }
			// Destructor
			~MatIterator() { }; // Nothing to Do
			// Assignment Operator
			// https://en.cppreference.com/w/cpp/language/operators#Assignment_operator
			constexpr self_type& operator=(const self_type& other){
				if(this != &other){
					ptr = other.ptr;
					present = other.present;
					pos = other.pos;
				}
				return (*this);
			}
			constexpr self_type& operator=(const reference other){
				*present = other;
				return (*this);
			}
			constexpr self_type& operator=(value_type other) {
				*present = std::move(other);
				return (*this);
			}
			constexpr self_type& operator++(){ ++pos; update_ptr(); return (*this); }
			constexpr self_type operator++(int){ self_type tmp = *this; ++(*this); return tmp; }
			constexpr self_type& operator--(){ --pos; update_ptr(); return (*this); }
			constexpr self_type operator--(int){ self_type tmp = *this; --(*this); return tmp; }
			constexpr reference operator*() { return *present; }
			constexpr bool operator==(const self_type& rhs) const { return (pos == rhs.pos); }
			constexpr bool operator!=(const self_type& rhs) const { return (pos != rhs.pos); }
		private:
			T* const ptr;
			T *present; // Cache the access location
			size_type pos;
			static_assert(std::numeric_limits<size_type>::max() >
				(ARRAY_ELEMENT_LENGTH*ARRAY_ELEMENT_LENGTH + 1),
				"Position tracker is not large enough.");
			constexpr void update_ptr(){
				present = ptr + (ARRAY_TOTAL_WIDTH*(pos >> ARRAY_ELEMENT_POWER) +
					(pos & (ARRAY_ELEMENT_LENGTH - 1)));
			}
		};
		constexpr MatIterator begin() {
			return MatIterator(&dat[1][1], 0);
		}
		constexpr MatIterator end() {
			return MatIterator(&dat[1][1], ARRAY_ELEMENT_LENGTH*ARRAY_ELEMENT_LENGTH);
		}
	/* ==== END (Matrix ITERATOR IMPLEMENTATION) === */
		/* Data Manipulation :: Populate Ghost Cells */
		constexpr void fill_ghost_top(const Array2D& neighbor){
			// Copies the bottom row of Neighbor into the top ghost row
			// Assumes Neighbor is directly above (this)
			for(auto ii = 1; ii < (ARRAY_ELEMENT_LENGTH + 1); ++ii){
				dat[0][ii] = std::copy(neighbor.dat[ARRAY_ELEMENT_LENGTH][ii]);				
			}
		}
		constexpr void fill_ghost_top( T(&&source)[ARRAY_ELEMENT_LENGTH] ){
			// Overloaded Method, Consumes an array of elements
			for(auto ii = 0; ii < ARRAY_ELEMENT_LENGTH; ++ii){
				dat[0][ii+1] = std::move(source[ii]);
			}
		}
		constexpr void fill_ghost_bottom(const Array2D& neighbor){
			// Copies the top row of Neighbor into the bottom ghost row
			// Assumes Neighbor is directly below (this)
			for(auto ii = 1; ii < (ARRAY_ELEMENT_LENGTH + 1); ++ii){
				dat[ARRAY_ELEMENT_LENGTH+1][ii] = std::copy(neighbor.dat[1][ii]);				
			}
		}
		constexpr void fill_ghost_bottom( T(&&source)[ARRAY_ELEMENT_LENGTH] ){
			// Overloaded Method, Consumes an array of elements
			for(auto ii = 0; ii < ARRAY_ELEMENT_LENGTH; ++ii){
				dat[ARRAY_ELEMENT_LENGTH+1][ii+1] = std::move(source[ii]);
			}
		}
		constexpr void fill_ghost_left(const Array2D& neighbor){
			// Copies the rightmost column of Neighbor into the left ghost column
			// Assumes Neighbor is directly left of (this)
			for(auto ii = 1; ii < (ARRAY_ELEMENT_LENGTH + 1); ++ii){
				dat[ii][0] = std::copy(neighbor.dat[ii][ARRAY_ELEMENT_LENGTH]);				
			}
		}
		constexpr void fill_ghost_left( T(&&source)[ARRAY_ELEMENT_LENGTH] ){
			// Overloaded Method, Consumes an array of elements
			for(auto ii = 0; ii < ARRAY_ELEMENT_LENGTH; ++ii){
				dat[ii+1][0] = std::move(source[ii]);
			}
		}
		constexpr void set_ghost_right(const Array2D& neighbor){
			// Copies the rightmost column of Neighbor into the left ghost column
			// Assumes Neighbor is directly right of (this)
			for(auto ii = 1; ii < (ARRAY_ELEMENT_LENGTH + 1); ++ii){
				dat[ii][ARRAY_ELEMENT_LENGTH+1] = std::copy(neighbor.dat[ii][0]);				
			}
		}
		constexpr void set_ghost_right( T(&&source)[ARRAY_ELEMENT_LENGTH] ){
			// Overloaded Method, Consumes an array of elements
			for(auto ii = 0; ii < ARRAY_ELEMENT_LENGTH; ++ii){
				dat[ii+1][ARRAY_ELEMENT_LENGTH+1] = std::move(source[ii]);
			}
		}
		/* CPP Operators */
		constexpr friend std::ostream& operator<<(std::ostream &stream, const Array2D<T> &array){
			stream << std::fixed << std::setprecision(0);
			for(int_fast8_t ii = 1; ii < ARRAY_ELEMENT_LENGTH + 1; ++ii){
				for(int_fast8_t jj = 1; jj < ARRAY_ELEMENT_LENGTH + 1 ; ++jj){
					stream << std::setw(6) << array.dat[ii][jj];
				}
				stream << "\n";
			}
			stream << std::setprecision(4);
			stream.unsetf(std::ios_base::floatfield);
			return stream;
		}
	private:
		constexpr std::ptrdiff_t query_bounds(const std::ptrdiff_t position){
			return (position & (ARRAY_ELEMENT_LENGTH - 1)) + 1;
		}
		// Data
		T dat[ARRAY_ELEMENT_LENGTH + 2][ARRAY_ELEMENT_LENGTH + 2];
		/** Ghost Cells & Base data ared store in a continuous block.
		 *  This is to provided simplification of processing code, but comes
		 *  at a minor storage penalty of 4 wasted elements/object
		 *  Ghost Elements are Stored along the perimenter to preserve spatial
		 *  relationship between elements.
		 *  This does come at the penalty of needing additional redirection.
		 **/
		// Methods
	};

#endif // ARRAY_2D_H_
