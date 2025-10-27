#ifndef MAIN_H
#define MAIN_H

//------------------------- Important Libraries ------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>

#include <math.h>
#include <float.h>

// ------------------------User - Defined Libraries --------------------------
#include "parseInput.h"
#include "utils.h"

#include "hashmap.h"
#include "datasets.h"  // Must come before lsh.h and hypercube.h (defines DataType)

#include "hashtable.h"
#include "lsh.h"

#include "query.h"
#include "runAlgorithms.h"


#include "hypercube.h"
#include "ivfflat.h"

#endif