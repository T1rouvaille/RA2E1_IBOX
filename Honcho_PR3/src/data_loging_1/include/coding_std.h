//
// Created by GMB1226 on 11/21/2018.
//
/*
 * This file contains examples and conventions for common c-style idioms to be honored when developing SBD software
 *
 * v1.0: Initial version
 */
#ifndef PROJECT_SBD_CODING_STANDARD_H
#define PROJECT_SBD_CODING_STANDARD_H

/*
 * ------------------------------------------------------------------------------------------------
 * Naming Conventions:
 *     File: all lowercase, with underscore between words
 *         ex: sbd_coding_standard.h
 *
 *     Types: all new types must be lowercase and end in *_t
 *         ex: timer_t
 *
 *     Functions: Uppercase with underscores. Make an effort to limit fn to <50 lines of code
 *                Should only have one return point
 *                Prefer inline fn over macro, if possible
 *         ex: void This_Is_My_Function();
 *
 *     Macros: Macros shall only contain UPPERCASE
 *             Preprocessor macro usage should follow indenting rules:
 *         ex: #ifndef DEFAULT_TRIGGER_THRESHOLD
 *                 #define DEFAULT_TRIGGER_THRESHOLD 50
 *             #endif
 *
 *             Configurable preprocessor tokens should be prefixed with DEFAULT_
 *         ex: #define DEFAULT_TRIGGER_THREHSOLD 25
 *
 *     Interrupt functions: Must end with *_isr.
 *                          Must declare static and place at bottom of driver, or bottom
 *                              of source file.
 *                          All unused interrupts should be implemented and attempt to
 *                              disable itself or fail an assert check
 *         ex: void ADC_Result_Handler_isr();
 *
 *     Variables: Should not begin name with _
 *                Name length must be 3 < Length < 32
 *                Lowercase with underscores
 *                Do not include type information in var name
 *                All global variables must be prefixed with g_*
 *                All pointers must be prefixed with p_*
 *                    Similarly, pointer to pointer is pp_*
 *                All boolean or boolean-like integers mst be prefixed with b_*
 *                Each variable must be declared on its own line. No commas
 *                Minor whitespace adjustments to align declarations and definitions is acceptable
 *         ex: int32_t  this_is_my_variable;
 *             uint32_t this_is_another_variable = 1;
 * ------------------------------------------------------------------------------------------------
 *
 * Other formatting:
 *     Tabs: NO TABS ANYWHERE, EVER. Disable tabs in your IDE of choice
 *
 *     Indentation: 4 spaces. NO TABS
 *
 *     Linefeeds: Use LF only. Convert all CR-LF to LF
 *
 *     Line width: 100 characters
 *
 *     Braces: Below the block it contains
 *         ex: if (b_earth_stops_rotating)
 *             {
 *                 do_panic();
 *             }
 *
 *     Parentheses: Explicitly enforce order of operations
 *         ex: if ((thing1 + 10) * 2)
 *             ...
 *
 *     if statements: Place any constant in the left hand operand
 *         ex: if (40 >= my_age)
 *             {
 *                 have_mid_life_crisis();
 *             }
 * ------------------------------------------------------------------------------------------------
 *
 *
 * Pointer Syntax
 *     Pointer: Since the room was split on left-hand or right-hand *, we put it in the middle
 *         ex: uint32_t * p_len;
 * ------------------------------------------------------------------------------------------------
 *
 * Casting
 *     Typecasts: Every typecast must contain a comment describing how all possible right-side
 *                expression values are handled
 * ------------------------------------------------------------------------------------------------
 *
 *
 * Keywords to avoid:
 *     auto, register, goto, continue, break (unless inside a switch)
 */

#endif //PROJECT_SBD_CODING_STANDARD_H
