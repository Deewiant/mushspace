// File created: 2011-08-09 19:22:21

#include "cell.h"

mushcell mushcell_max(mushcell a, mushcell b) { return a > b ? a : b; }
mushcell mushcell_min(mushcell a, mushcell b) { return a < b ? a : b; }

mushcell mushcell_add(mushcell a, mushcell b) {
	return (mushcell)((mushucell)a + (mushucell)b);
}
mushcell mushcell_sub(mushcell a, mushcell b) {
	return (mushcell)((mushucell)a - (mushucell)b);
}
mushcell mushcell_inc(mushcell n) {
	return mushcell_add(n, 1);
}
