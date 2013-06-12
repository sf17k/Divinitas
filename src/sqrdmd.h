/** @file sqrdmd.h
 *
 *  @author Lauri Viitanen
 *  @date 2011-08-09
 */
#ifndef SQRDMD_H
#define SQRDMD_H

#ifdef	__cplusplus
extern "C" {
#endif

/**
 *  @brief Generates a two dimensional fractal height map.
 *
 *  Function calculates fractal values into given array with square-diamond
 *  algorithm. Values other than zero in target array are left unmodified.
 *  The gradient between each element of smoothness of map can be controlled
 *  with 'rgh' parameter so that value 0.0f produces completely flat/smooth
 *  map and value 1.0f produces completely random (noise) map.
 *
 *  @param	map Destination array to store the results.
 *  @param	size Length of map's side: 2^x + 1, x = 1, 2, 3 ...
 *  @param	rgh Amount of roughness/randomness in the final map.
 *  @return	Returns zero on success.
 */
extern int sqrdmd(float* map, int size, float rgh);

#ifdef	__cplusplus
}
#endif

#endif
