/* praat_FFNet_init.cpp
 *
 * Copyright (C) 1994-2011, 2016 David Weenink
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This code is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this work. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 djmw 20020408 GPL
 djmw 20020408 added FFNet_help
 djmw 20030701 Removed non-GPL minimizations
 djmw 20040526 Removed bug in FFNet_drawCostHistory interface.
 djmw 20041123 Latest modification
 djmw 20061218 Changed to Melder_information<x> format.
 djmw 20080902 Melder_error<1...>
 djmw 20071011 REQUIRE requires U"".
 djmw 20071024 Use MelderString_append in FFNet_createNameFromTopology
 djmw 20100511 FFNet query outputs
*/

#include <math.h>
#include "praat.h"
#include "Discriminant.h"
#include "PCA.h"
#include "Minimizers.h"
#include "FFNet_Eigen.h"
#include "FFNet_Matrix.h"
#include "FFNet_PatternList.h"
#include "FFNet_ActivationList_Categories.h"
#include "FFNet_PatternList_ActivationList.h"
#include "FFNet_PatternList_Categories.h"
#include "RBM_extensions.h"

/* Routines to be removed sometime in the future:
20040422, 2.4.04: FFNet_drawWeightsToLayer  use FFNet_drawWeights
20040422, 2.4.04: FFNet_weightsToMatrix use FFNet_extractWeights
*/

#undef iam
#define iam iam_LOOP

static char32 const *QUERY_BUTTON   = U"Query -";
static char32 const *DRAW_BUTTON     = U"Draw -";
static char32 const *MODIFY_BUTTON  = U"Modify -";
static char32 const *EXTRACT_BUTTON = U"Extract -";

/**************** New FFNet ***************************/

static void FFNet_create_addCommonFields_inputOutput (UiForm dia) {
	NATURAL (U"Number of inputs", U"4")
	NATURAL (U"Number of outputs", U"3")
}

static void FFNet_create_checkCommonFields_inputOutput (UiForm dia, long *numberOfInputs, long *numberOfOutputs) {
	*numberOfInputs = GET_INTEGER (U"Number of inputs");
	*numberOfOutputs = GET_INTEGER (U"Number of outputs");
}

static void FFNet_create_addCommonFields_hidden (UiForm dia) {
	INTEGER (U"Number of units in hidden layer 1", U"0")
	INTEGER (U"Number of units in hidden layer 2", U"0")
}

static void FFNet_create_checkCommonFields_hidden (UiForm dia, 	long *numberOfHidden1, long *numberOfHidden2) {
	*numberOfHidden1 = GET_INTEGER (U"Number of units in hidden layer 1");
	*numberOfHidden2 = GET_INTEGER (U"Number of units in hidden layer 2");
	if (*numberOfHidden1 < 0 || *numberOfHidden2 < 0) {
		Melder_throw (U"The number of units in a hidden layer must be geater equal zero");
	}
}

static void FFNet_create_addCommonFields (UiForm dia) {
	FFNet_create_addCommonFields_inputOutput (dia);
	FFNet_create_addCommonFields_hidden (dia);
}

static void FFNet_create_checkCommonFields (UiForm dia, long *numberOfInputs, long *numberOfOutputs,
        long *numberOfHidden1, long *numberOfHidden2) {
	FFNet_create_checkCommonFields_inputOutput (dia, numberOfInputs, numberOfOutputs);
	FFNet_create_checkCommonFields_hidden (dia, numberOfHidden1, numberOfHidden2);
}

FORM (FFNet_create, U"Create FFNet", U"Create FFNet...") {
	WORD (U"Name", U"4-3")
	FFNet_create_addCommonFields (dia);
	OK2
DO
	long numberOfInputs, numberOfOutputs, numberOfHidden1, numberOfHidden2;
	FFNet_create_checkCommonFields (dia, &numberOfInputs, &numberOfOutputs, &numberOfHidden1, &numberOfHidden2);
	autoFFNet thee = FFNet_create (numberOfInputs, numberOfHidden1, numberOfHidden2, numberOfOutputs, false);
	praat_new (thee.move(), GET_STRING (U"Name"));
END2 }

FORM (FFNet_create_linearOutputs, U"Create FFNet", U"Create FFNet (linear outputs)...") {
	WORD (U"Name", U"4-3L")
	FFNet_create_addCommonFields (dia);
	OK2
DO
	long numberOfInputs, numberOfOutputs, numberOfHidden1, numberOfHidden2;
	FFNet_create_checkCommonFields (dia, &numberOfInputs, &numberOfOutputs, &numberOfHidden1, &numberOfHidden2);
	autoFFNet thee = FFNet_create (numberOfInputs, numberOfHidden1, numberOfHidden2, numberOfOutputs, true);
	praat_new (thee.move(), GET_STRING (U"Name"));
END2 }

FORM (FFNet_createIrisExample, U"Create iris example", U"Create iris example...") {
	LABEL (U"", U"For the feedforward neural net we need to know the:")
	FFNet_create_addCommonFields_hidden (dia);
	OK2
DO
	long numberOfHidden1, numberOfHidden2;
	FFNet_create_checkCommonFields_hidden (dia, &numberOfHidden1, &numberOfHidden2);
	autoCollection thee = FFNet_createIrisExample (numberOfHidden1, numberOfHidden2);
	praat_new (thee.move());
END2 }

DIRECT2 (FFNet_getNumberOfInputs) {
	LOOP {
		iam (FFNet);
		Melder_information (my nUnitsInLayer[0], U" units");
	}
END2 }

DIRECT2 (FFNet_getNumberOfOutputs) {
	LOOP {
		iam (FFNet);
		Melder_information (my nUnitsInLayer[my nLayers], U" units");
	}
END2 }

FORM (FFNet_getNumberOfHiddenUnits, U"FFNet: Get number of hidden units", U"FFNet: Get number of hidden units...") {
	NATURAL (U"Hidden layer number", U"1")
	OK2
DO
	LOOP {
		iam (FFNet);
		long layerNumber = GET_INTEGER (U"Hidden layer number");
		long numberOfUnits = 0;
		if (layerNumber > 0 && layerNumber <= my nLayers - 1) {
			numberOfUnits = my nUnitsInLayer[layerNumber];
		}
		Melder_information (numberOfUnits, U" units");
	}
END2 }

FORM (FFNet_getCategoryOfOutputUnit, U"FFNet: Get category of output unit", nullptr) {
	NATURAL (U"Output unit", U"1")
	OK2
DO
	LOOP {
		iam (FFNet);
		long unit = GET_INTEGER (U"Output unit");
		if (unit > my outputCategories->size) {
			Melder_throw (U"Output unit cannot be larger than ", my outputCategories->size, U".");
		}
		SimpleString ss = my outputCategories->at [unit];
		Melder_information (ss -> string);
	}
END2 }

FORM (FFNet_getOutputUnitOfCategory, U"FFNet: Get output unit of category", nullptr) {
	SENTENCE (U"Category", U"u")
	OK2
DO
	LOOP {
		iam (FFNet);
		char32 *category = GET_STRING (U"Category");
		long index = 0;
		for (long i = 1; i <= my outputCategories->size; i ++) {
			SimpleString s = my outputCategories->at [i];
			if (Melder_equ (s -> string, category)) {
				index = i;
				break;
			}
		}
		Melder_information (index);
	}
END2 }

FORM (FFNet_getBias, U"FFNet: Get bias", nullptr) {
	NATURAL (U"Layer", U"1")
	NATURAL (U"Unit", U"1")
	OK2
DO
	LOOP {
		iam (FFNet);
		long layer = GET_INTEGER (U"Layer");
		long unit = GET_INTEGER (U"Unit");
		double bias = FFNet_getBias (me, layer, unit);
		Melder_information (bias, U"(bias for unit ", unit, U"in layer ", layer, U").");
	}
END2 }

FORM (FFNet_setBias, U"FFNet: Set bias", nullptr) {
	NATURAL (U"Layer", U"1")
	NATURAL (U"Unit", U"1")
	REAL (U"Value", U"0.0")
	OK2
DO
	LOOP {
		iam (FFNet);
		FFNet_setBias (me, GET_INTEGER (U"Layer"), GET_INTEGER (U"Unit"), GET_REAL (U"Value"));
	}
END2 }

FORM (FFNet_getWeight, U"FFNet: Get weight", nullptr) {
	NATURAL (U"Layer", U"1")
	NATURAL (U"Unit", U"1")
	NATURAL (U"Unit from", U"1")
	OK2
DO
	LOOP {
		iam (FFNet);
		long layer = GET_INTEGER (U"Layer");
		long unit = GET_INTEGER (U"Unit");
		long unitf = GET_INTEGER (U"Unit from");
		double w = FFNet_getWeight (me, layer, unit, unitf);
		Melder_information (w, U"(weight between unit ", unit,
			U" in layer ", layer, U", and unit ", unitf, U"in layer ",
			layer - 1);
	}
END2 }

FORM (FFNet_setWeight, U"FFNet: Set weight", nullptr) {
	NATURAL (U"Layer", U"1")
	NATURAL (U"Unit", U"1")
	NATURAL (U"Unit (from)", U"1")
	REAL (U"Value", U"0.0")
	OK2
DO
	LOOP {
		iam (FFNet);
		long layer = GET_INTEGER (U"Layer");
		long unit = GET_INTEGER (U"Unit");
		long unitf = GET_INTEGER (U"Unit (from)");
		FFNet_setWeight (me, layer, unit, unitf, GET_REAL (U"Value"));
	}
END2 }

FORM (FFNet_getNumberOfHiddenWeights, U"FFNet: Get number of hidden weights", U"FFNet: Get number of hidden weights...") {
	NATURAL (U"Hidden layer number", U"1")
	OK2
DO
	LOOP {
		iam (FFNet);
		long layerNumber = GET_INTEGER (U"Hidden layer number");
		long numberOfWeights = 0;
		if (layerNumber > 0 && layerNumber <= my nLayers - 1) {
			numberOfWeights = my nUnitsInLayer[layerNumber] * (my nUnitsInLayer[layerNumber - 1] + 1);
		}
		Melder_information (numberOfWeights, U" weights (including biases)");
	}
END2 }

DIRECT2 (FFNet_getNumberOfOutputWeights) {
	LOOP {
		iam (FFNet);
		Melder_information (my nUnitsInLayer[my nLayers] * (my nUnitsInLayer[my nLayers - 1] + 1),
		U" weights");
	}
END2 }

/**************** New PatternList ***************************/

FORM (PatternList_create, U"Create PatternList", nullptr) {
	WORD (U"Name", U"1x1")
	NATURAL (U"Dimension of a pattern", U"1")
	NATURAL (U"Number of patterns", U"1")
	OK2
DO
	praat_new (PatternList_create (GET_INTEGER (U"Number of patterns"),
		GET_INTEGER (U"Dimension of a pattern")), GET_STRING (U"Name"));
END2 }

/**************** New Categories ***************************/

FORM (Categories_create, U"Create Categories", nullptr) {
	WORD (U"Name", U"empty")
	OK2
DO
	autoCategories thee = Categories_create ();
	praat_new (thee.move(), GET_STRING (U"Name"));
END2 }

DIRECT2 (FFNet_help)  {
	Melder_help (U"Feedforward neural networks"); 
END2 }

DIRECT2 (FFNet_getMinimum) {
	LOOP {
		iam (FFNet);
		Melder_information (FFNet_getMinimum (me));
	}
END2 }

FORM (FFNet_reset, U"FFNet: Reset", U"FFNet: Reset...") {
	LABEL (U"", U"Warning: this command destroys all previous learning.")
	LABEL (U"", U"New weights will be randomly chosen from the interval [-range, +range].")
	POSITIVE (U"Range", U"0.1")
	OK2
DO
	LOOP {
		iam (FFNet);
		FFNet_reset (me, GET_REAL (U"Range"));
		praat_dataChanged (OBJECT);
	}
END2 }

FORM (FFNet_selectBiasesInLayer, U"FFNet: Select biases", U"FFNet: Select biases...") {
	LABEL (U"", U"WARNING: This command induces very specific behaviour ")
	LABEL (U"", U"during a following learning phase.")
	NATURAL (U"Layer number", U"1")
	OK2
DO
	LOOP {
		iam (FFNet);
		FFNet_selectBiasesInLayer (me, GET_INTEGER (U"Layer number"));
		praat_dataChanged (OBJECT);
	}
END2 }

DIRECT2 (FFNet_selectAllWeights) {
	LOOP {
		iam (FFNet);
		FFNet_selectAllWeights (me);
		praat_dataChanged (me);
	}
END2 }

DIRECT2 (FFNet_drawTopology) {
	autoPraatPicture picture;
	LOOP {
		iam (FFNet);
		FFNet_drawTopology (me, GRAPHICS);
	}
END2 }

FORM (FFNet_drawWeightsToLayer, U"FFNet: Draw weights to layer", nullptr) {
	LABEL (U"", U"Warning: Disapproved. Use \"Draw weights..\" instead.")
	NATURAL (U"Layer number", U"1")
	RADIO (U"Scale", 1)
		RADIOBUTTON (U"by maximum of all weights to layer")
		RADIOBUTTON (U"by maximum weight from 'from-unit'")
		RADIOBUTTON (U"by maximum weight to 'to-unit'")
	BOOLEAN (U"Garnish", true)
	OK2
DO
	autoPraatPicture picture;
	LOOP {
		iam (FFNet);
		FFNet_drawWeightsToLayer (me, GRAPHICS, GET_INTEGER (U"Layer number"),
			GET_INTEGER (U"Scale"), GET_INTEGER (U"Garnish"));
	}
END2 }

FORM (FFNet_drawWeights, U"FFNet: Draw weights", U"FFNet: Draw weights...") {
	NATURAL (U"Layer number", U"1")
	BOOLEAN (U"Garnish", true)
	OK2
DO
	autoPraatPicture picture;
	LOOP {
		iam (FFNet);
		FFNet_drawWeights (me, GRAPHICS, GET_INTEGER (U"Layer number"), GET_INTEGER (U"Garnish"));
	}
END2 }

FORM (FFNet_drawCostHistory, U"FFNet: Draw cost history", U"FFNet: Draw cost history...") {
	INTEGER (U"left Iteration_range", U"0")
	INTEGER (U"right Iteration_range", U"0")
	REAL (U"left Cost_range", U"0.0")
	REAL (U"right Cost_range", U"0.0")
	BOOLEAN (U"Garnish", true)
	OK2
DO
	autoPraatPicture picture;
	LOOP {
		iam (FFNet);
		FFNet_drawCostHistory (me, GRAPHICS, GET_INTEGER (U"left Iteration_range"),
			GET_INTEGER (U"right Iteration_range"), GET_REAL (U"left Cost_range"), GET_REAL (U"right Cost_range"),
			GET_INTEGER (U"Garnish"));
	}
END2 }

FORM (FFNet_extractWeights, U"FFNet: Extract weights", U"FFNet: Extract weights...") {
	NATURAL (U"Layer number", U"1")
	OK2
DO
	LOOP {
		iam (FFNet);
		autoTableOfReal thee = FFNet_extractWeights (me, GET_INTEGER (U"Layer number"));
		praat_new (thee.move());
	}
END2 }

FORM (FFNet_weightsToMatrix, U"FFNet: Weights to Matrix ", nullptr) {
	LABEL (U"", U"Warning: Use \"Extract weights..\" instead.")
	NATURAL (U"Layer number", U"1")
	OK2
DO
	LOOP {
		iam (FFNet);
		autoMatrix thee = FFNet_weightsToMatrix (me, GET_INTEGER (U"Layer number"), false);
		praat_new (thee.move(), my name);
	}
END2 }

/******************* FFNet && ActivationList *************************************/

FORM (FFNet_ActivationList_to_Categories, U"FFNet & ActivationList: To Categories", 0) {
	RADIO (U"Kind of labeling", 1)
	RADIOBUTTON (U"Winner-takes-all")
	RADIOBUTTON (U"Stochastic")
	OK2
DO
	FFNet me = FIRST (FFNet);
	ActivationList thee = FIRST (ActivationList);
	autoCategories him = FFNet_ActivationList_to_Categories (me, thee, GET_INTEGER (U"Kind of labeling"));
	praat_new (him.move(), my name, U"_", thy name);
END2 }

/******************* FFNet && Eigen ******************************************/

FORM (FFNet_Eigen_drawIntersection, U"FFnet & Eigen: Draw intersection", 0) {
	INTEGER (U"X-component", U"1")
	INTEGER (U"Y-component", U"2")
	REAL (U"xmin", U"0.0")
	REAL (U"xmax", U"0.0")
	REAL (U"ymin", U"0.0")
	REAL (U"ymax", U"0.0")
	OK2
DO
	FFNet me = FIRST (FFNet);
	Eigen thee = FIRST (Eigen);
	autoPraatPicture picture;
	long pcx = GET_INTEGER (U"X-component");
	long pcy = GET_INTEGER (U"Y-component");
	REQUIRE (pcx != 0 && pcy != 0, U"X and Y component must differ from 0.")
	FFNet_Eigen_drawIntersection (me, thee, GRAPHICS, pcx, pcy, GET_REAL (U"xmin"), GET_REAL (U"xmax"),
		GET_REAL (U"ymin"), GET_REAL (U"ymax"));
END2 }

FORM (FFNet_PCA_drawDecisionPlaneInEigenspace, U"FFNet & PCA: Draw decision plane", nullptr) {
	NATURAL (U"Unit number", U"1")
	NATURAL (U"Layer number", U"1")
	NATURAL (U"Horizontal eigenvector number", U"1")
	NATURAL (U"Vertical eigenvector number", U"2")
	REAL (U"left Horizontal range", U"0.0")
	REAL (U"right Horizontal range", U"0.0")
	REAL (U"left Vertical range", U"0.0")
	REAL (U"right Vertical range", U"0.0")
	OK2
DO
	autoPraatPicture picture;
	FFNet me = FIRST (FFNet);
	PCA thee = FIRST (PCA);
	FFNet_Eigen_drawDecisionPlaneInEigenspace (me, thee,
		GRAPHICS, GET_INTEGER (U"Unit number"), GET_INTEGER (U"Layer number"),
		GET_INTEGER (U"Horizontal eigenvector number"),
		GET_INTEGER (U"Vertical eigenvector number"), GET_REAL (U"left Horizontal range"),
		GET_REAL (U"right Horizontal range"), GET_REAL (U"left Vertical range"),
		GET_REAL (U"right Vertical range"));
END2 }

/************************* FFNet && Categories **********************************/

DIRECT2 (FFNet_Categories_to_ActivationList) {
	FFNet me = FIRST (FFNet);
	Categories thee = FIRST (Categories);
	autoActivationList him = FFNet_Categories_to_ActivationList (me, thee);
	praat_new (him.move(), my name);
END2 }

/************************* FFNet && Matrix **********************************/

FORM (FFNet_weightsFromMatrix, U"Replace weights by values from Matrix", nullptr) {
	NATURAL (U"Layer", U"1")
	OK2
DO
	FFNet me = FIRST (FFNet);
	Matrix thee = FIRST (Matrix);
	autoFFNet him = FFNet_weightsFromMatrix (me, thee, GET_INTEGER (U"Layer"));
	praat_new (him.move(), my name);
END2 }

/************************* FFNet && PatternList **********************************/

FORM (FFNet_PatternList_drawActivation, U"Draw an activation", nullptr) {
	NATURAL (U"PatternList (row) number", U"1");
	OK2
DO
	autoPraatPicture picture;
	FFNet me = FIRST (FFNet);
	PatternList thee = FIRST (PatternList);
	FFNet_PatternList_drawActivation (me, thee, GRAPHICS, GET_INTEGER (U"PatternList"));
END2 }

FORM (FFNet_PatternList_to_ActivationList, U"To activations in layer", nullptr) {
	NATURAL (U"Layer", U"1")
	OK2
DO
	FFNet me = FIRST (FFNet);
	PatternList thee = FIRST (PatternList);
	autoActivationList him = FFNet_PatternList_to_ActivationList (me, thee, GET_INTEGER (U"Layer"));
	praat_new (him.move(), my name, U"_", thy name);
END2 }

DIRECT2 (hint_FFNet_and_PatternList_classify) {
	Melder_information (U"You can use the FFNet as a classifier by selecting a\n"
		"FFNet and a PatternList together and choosing \"To Categories...\".");
END2 }

DIRECT2 (hint_FFNet_and_PatternList_and_Categories_learn) {
	Melder_information (U"You can teach a FFNet to classify by selecting a\n"
		"FFNet, a PatternList and a Categories together and choosing \"Learn...\".");
END2 }

FORM (FFNet_PatternList_to_Categories, U"FFNet & PatternList: To Categories", U"FFNet & PatternList: To Categories...") {
	RADIO (U"Determine output category as", 1)
		RADIOBUTTON (U"Winner-takes-all")
		RADIOBUTTON (U"Stochastic")
	OK2
DO
	FFNet me = FIRST (FFNet);
	PatternList thee = FIRST (PatternList);
	autoCategories him = FFNet_PatternList_to_Categories (me, thee, GET_INTEGER (U"Determine output category as"));
	praat_new (him.move(), my name, U"_", thy name);
END2 }

/*********** FFNet PatternList ActivationList **********************************/

FORM (FFNet_PatternList_ActivationList_getCosts_total, U"FFNet & PatternList & ActivationList: Get total costs", U"FFNet & PatternList & ActivationList: Get total costs...") {
	RADIO (U"Cost function", 1)
	RADIOBUTTON (U"Minimum-squared-error")
	RADIOBUTTON (U"Minimum-cross-entropy")
	OK2
DO
	FFNet me = FIRST (FFNet);
	PatternList thee = FIRST (PatternList);
	ActivationList him = FIRST (ActivationList);
	Melder_information (FFNet_PatternList_ActivationList_getCosts_total (me, thee, him, GET_INTEGER (U"Cost function")));
END2 }

FORM (FFNet_PatternList_ActivationList_getCosts_average, U"FFNet & PatternList & ActivationList: Get average costs", U"FFNet & PatternList & ActivationList: Get average costs...") {
	RADIO (U"Cost function", 1)
	RADIOBUTTON (U"Minimum-squared-error")
	RADIOBUTTON (U"Minimum-cross-entropy")
	OK2
DO
	FFNet me = FIRST (FFNet);
	PatternList thee = FIRST (PatternList);
	ActivationList him = FIRST (ActivationList);
	Melder_information (FFNet_PatternList_ActivationList_getCosts_average (me, thee, him, GET_INTEGER (U"Cost function")));
END2 }

FORM (FFNet_PatternList_ActivationList_learnSD, U"FFNet & PatternList & ActivationList: Learn slow", nullptr) {
	NATURAL (U"Layer", U"1")
	NATURAL (U"Maximum number of epochs", U"100")
	POSITIVE (U"Tolerance of minimizer", U"1e-7")
	LABEL (U"Specifics", U"Specific for this minimization")
	POSITIVE (U"Learning rate", U"0.1")
	REAL (U"Momentum", U"0.9")
	RADIO (U"Cost function", 1)
		RADIOBUTTON (U"Minimum-squared-error")
		RADIOBUTTON (U"Minimum-cross-entropy")
	OK2
DO
	FFNet me = FIRST (FFNet);
	PatternList thee = FIRST (PatternList);
	ActivationList him = FIRST (ActivationList);
	return FFNet_PatternList_ActivationList_learnSD (me, thee, him, GET_INTEGER (U"Maximum number of epochs"),
			GET_REAL (U"Tolerance of minimizer"), GET_REAL (U"Learning rate"), GET_REAL (U"Momentum"), GET_INTEGER (U"Cost function"));
END2 }

FORM (FFNet_PatternList_ActivationList_learnSM, U"FFNet & PatternList & ActivationList: Learn", nullptr) {
	NATURAL (U"Layer", U"1")
	NATURAL (U"Maximum number of epochs", U"100")
	POSITIVE (U"Tolerance of minimizer", U"1e-7")
	RADIO (U"Cost function", 1)
		RADIOBUTTON (U"Minimum-squared-error")
		RADIOBUTTON (U"Minimum-cross-entropy")
	OK2
DO
	FFNet me = FIRST (FFNet);
	PatternList thee = FIRST (PatternList);
	ActivationList him = FIRST (ActivationList);
	return FFNet_PatternList_ActivationList_learnSM (me, thee, him, GET_INTEGER (U"Maximum number of epochs"),
		GET_REAL (U"Tolerance of minimizer"), GET_INTEGER (U"Cost function"));
END2 }

/*********** FFNet PatternList Categories **********************************/

FORM (FFNet_PatternList_Categories_getCosts_total, U"FFNet & PatternList & Categories: Get total costs", U"FFNet & PatternList & Categories: Get total costs...") {
	RADIO (U"Cost function", 1)
		RADIOBUTTON (U"Minimum-squared-error")
		RADIOBUTTON (U"Minimum-cross-entropy")
	OK2
DO
	FFNet me = FIRST (FFNet);
	PatternList thee = FIRST (PatternList);
	Categories him = FIRST (Categories);
	Melder_information (FFNet_PatternList_Categories_getCosts_total (me, thee, him,
		GET_INTEGER (U"Cost function")));
END2 }

FORM (FFNet_PatternList_Categories_getCosts_average, U"FFNet & PatternList & Categories: Get average costs", U"FFNet & PatternList & Categories: Get average costs...") {
	RADIO (U"Cost function", 1)
		RADIOBUTTON (U"Minimum-squared-error")
		RADIOBUTTON (U"Minimum-cross-entropy")
	OK2
DO
	FFNet me = FIRST (FFNet);
	PatternList thee = FIRST (PatternList);
	Categories him = FIRST (Categories);
	Melder_information (FFNet_PatternList_Categories_getCosts_average (me, thee, him,
		GET_INTEGER (U"Cost function")));
END2 }

FORM (PatternList_Categories_to_FFNet, U"PatternList & Categories: To FFNet", U"PatternList & Categories: To FFNet...") {
	INTEGER (U"Number of units in hidden layer 1", U"0")
	INTEGER (U"Number of units in hidden layer 2", U"0")
	OK2
DO
	PatternList me = FIRST (PatternList);
	Categories thee = FIRST (Categories);
	long nHidden1 = GET_INTEGER (U"Number of units in hidden layer 1");
	long nHidden2 = GET_INTEGER (U"Number of units in hidden layer 2");

	if (nHidden1 < 1) {
		nHidden1 = 0;
	}
	if (nHidden2 < 1) {
		nHidden2 = 0;
	}
	autoCategories uniq = Categories_selectUniqueItems (thee);
	long numberOfOutputs = uniq->size;
	if (numberOfOutputs < 1) Melder_throw (U"There are not enough categories in the Categories.\n"
		U"Please try again with more categories in the Categories.");

	autoFFNet ffnet = FFNet_create (my nx, nHidden1, nHidden2, numberOfOutputs, false);
	FFNet_setOutputCategories (ffnet.get(), uniq.get());
	autostring32 ffnetName = FFNet_createNameFromTopology (ffnet.get());
	praat_new (ffnet.move(), ffnetName.peek());
END2 }

FORM (FFNet_PatternList_Categories_learnSM, U"FFNet & PatternList & Categories: Learn", U"FFNet & PatternList & Categories: Learn...") {
	NATURAL (U"Maximum number of epochs", U"100")
	POSITIVE (U"Tolerance of minimizer", U"1e-7")
	RADIO (U"Cost function", 1)
		RADIOBUTTON (U"Minimum-squared-error")
		RADIOBUTTON (U"Minimum-cross-entropy")
	OK2
DO
	FFNet me = FIRST (FFNet);
	PatternList thee = FIRST (PatternList);
	Categories him = FIRST (Categories);
	FFNet_PatternList_Categories_learnSM (me, thee, him, GET_INTEGER (U"Maximum number of epochs"),
		GET_REAL (U"Tolerance of minimizer"), GET_INTEGER (U"Cost function"));
END2 }

FORM (FFNet_PatternList_Categories_learnSD, U"FFNet & PatternList & Categories: Learn slow", U"FFNet & PatternList & Categories: Learn slow...") {
	NATURAL (U"Maximum number of epochs", U"100")
	POSITIVE (U"Tolerance of minimizer", U"1e-7")
	LABEL (U"Specifics", U"Specific for this minimization")
	POSITIVE (U"Learning rate", U"0.1")
	REAL (U"Momentum", U"0.9")
	RADIO (U"Cost function", 1)
		RADIOBUTTON (U"Minimum-squared-error")
		RADIOBUTTON (U"Minimum-cross-entropy")
	OK2
DO
	FFNet me = FIRST (FFNet);
	PatternList thee = FIRST (PatternList);
	Categories him = FIRST (Categories);
	FFNet_PatternList_Categories_learnSD (me, thee, him, GET_INTEGER (U"Maximum number of epochs"),
		GET_REAL (U"Tolerance of minimizer"), GET_REAL (U"Learning rate"), GET_REAL (U"Momentum"), GET_INTEGER (U"Cost function"));
END2 }

DIRECT2 (RBM_PatternList_to_ActivationList) {
	iam_ONLY (RBM);
	youare_ONLY (PatternList);
	autoActivationList result = RBM_PatternList_to_ActivationList (me, you);
	praat_new (result.move(), my name, U"_", your name);
END2 }

void praat_uvafon_FFNet_init ();
void praat_uvafon_FFNet_init () {
	Thing_recognizeClassesByName (classFFNet, NULL);

	praat_addMenuCommand (U"Objects", U"New", U"Feedforward neural networks", nullptr, 0, nullptr);
	praat_addMenuCommand (U"Objects", U"New", U"Feedforward neural networks", nullptr, praat_DEPTH_1 | praat_NO_API, DO_FFNet_help);
	praat_addMenuCommand (U"Objects", U"New", U"-- FFNet --", nullptr, 1, nullptr);
	praat_addMenuCommand (U"Objects", U"New", U"Create iris example...", nullptr, 1, DO_FFNet_createIrisExample);
	praat_addMenuCommand (U"Objects", U"New", U"Create FFNet...", nullptr, 1, DO_FFNet_create);
	praat_addMenuCommand (U"Objects", U"New", U"Advanced", nullptr, 1, nullptr);
	praat_addMenuCommand (U"Objects", U"New", U"Create PatternList...", nullptr, 2, DO_PatternList_create);
	praat_addMenuCommand (U"Objects", U"New", U"Create Categories...", nullptr, 2, DO_Categories_create);
	praat_addMenuCommand (U"Objects", U"New", U"Create FFNet (linear outputs)...", nullptr, 2, DO_FFNet_create_linearOutputs);

	praat_addAction1 (classFFNet, 0, U"FFNet help", nullptr, praat_NO_API, DO_FFNet_help);
	praat_addAction1 (classFFNet, 0, DRAW_BUTTON, nullptr, 0, nullptr);
	praat_addAction1 (classFFNet, 0, U"Draw topology", nullptr, 1, DO_FFNet_drawTopology);
	praat_addAction1 (classFFNet, 0, U"Draw weights...", nullptr, 1, DO_FFNet_drawWeights);
	praat_addAction1 (classFFNet, 0, U"Draw weights to layer...", nullptr, praat_DEPTH_1 | praat_HIDDEN, DO_FFNet_drawWeightsToLayer);
	praat_addAction1 (classFFNet, 0, U"Draw cost history...", nullptr, 1, DO_FFNet_drawCostHistory);
	praat_addAction1 (classFFNet, 0, QUERY_BUTTON, nullptr, 0, nullptr);
	praat_addAction1 (classFFNet, 0, U"Query structure", nullptr, 1, nullptr);
	praat_addAction1 (classFFNet, 1, U"Get number of outputs", nullptr, 2, DO_FFNet_getNumberOfOutputs);
	praat_addAction1 (classFFNet, 1, U"Get number of hidden units...", nullptr, 2, DO_FFNet_getNumberOfHiddenUnits);
	praat_addAction1 (classFFNet, 1, U"Get number of inputs", nullptr, 2, DO_FFNet_getNumberOfInputs);
	praat_addAction1 (classFFNet, 1, U"Get number of hidden weights...", nullptr, 2, DO_FFNet_getNumberOfHiddenWeights);
	praat_addAction1 (classFFNet, 1, U"Get number of output weights", nullptr, 2, DO_FFNet_getNumberOfOutputWeights);
	praat_addAction1 (classFFNet, 1, U"Get category of output unit...", nullptr, 2, DO_FFNet_getCategoryOfOutputUnit);
	praat_addAction1 (classFFNet, 1, U"Get output unit of category...", nullptr, 2, DO_FFNet_getOutputUnitOfCategory);
	praat_addAction1 (classFFNet, 0, U"-- FFNet weights --", nullptr, 1, nullptr);
	praat_addAction1 (classFFNet, 1, U"Get bias...", nullptr, 1, DO_FFNet_getBias);
	praat_addAction1 (classFFNet, 1, U"Get weight...", nullptr, 1, DO_FFNet_getWeight);
	praat_addAction1 (classFFNet, 1, U"Get minimum", nullptr, 1, DO_FFNet_getMinimum);
	praat_addAction1 (classFFNet, 0, MODIFY_BUTTON, nullptr, 0, nullptr);
	praat_addAction1 (classFFNet, 1, U"Set bias...", nullptr, 1, DO_FFNet_setBias);
	praat_addAction1 (classFFNet, 1, U"Set weight...", nullptr, 1, DO_FFNet_setWeight);
	praat_addAction1 (classFFNet, 1, U"Reset...", nullptr, 1, DO_FFNet_reset);
	praat_addAction1 (classFFNet, 0, U"Select biases...", nullptr, 1, DO_FFNet_selectBiasesInLayer);
	praat_addAction1 (classFFNet, 0, U"Select all weights", nullptr, 1, DO_FFNet_selectAllWeights);
	praat_addAction1 (classFFNet, 0, EXTRACT_BUTTON, nullptr, 0, nullptr);
	praat_addAction1 (classFFNet, 0, U"Extract weights...", nullptr, 1, DO_FFNet_extractWeights);
	praat_addAction1 (classFFNet, 0, U"Weights to Matrix...", nullptr, praat_DEPTH_1 | praat_HIDDEN, DO_FFNet_weightsToMatrix);
	praat_addAction1 (classFFNet, 0, U"& PatternList: Classify?", nullptr, 0, DO_hint_FFNet_and_PatternList_classify);
	praat_addAction1 (classFFNet, 0, U"& PatternList & Categories: Learn?", nullptr, 0, DO_hint_FFNet_and_PatternList_and_Categories_learn);

	praat_addAction2 (classFFNet, 1, classActivationList, 1, U"Analyse", nullptr, 0, nullptr);
	praat_addAction2 (classFFNet, 1, classActivationList, 1, U"To Categories...", nullptr, 0, DO_FFNet_ActivationList_to_Categories);

	praat_addAction2 (classFFNet, 1, classEigen, 1, U"Draw", nullptr, 0, nullptr);
	praat_addAction2 (classFFNet, 1, classEigen, 1, U"Draw hyperplane intersections", nullptr, 0, DO_FFNet_Eigen_drawIntersection);

	praat_addAction2 (classFFNet, 1, classCategories, 1, U"Analyse", nullptr, 0, nullptr);
	praat_addAction2 (classFFNet, 1, classCategories, 1, U"To ActivationList", nullptr, 0, DO_FFNet_Categories_to_ActivationList);

	praat_addAction2 (classFFNet, 1, classMatrix, 1, U"Modify", nullptr, 0, nullptr);
	praat_addAction2 (classFFNet, 1, classMatrix, 1, U"Weights from Matrix...", nullptr, 0, DO_FFNet_weightsFromMatrix);

	praat_addAction2 (classFFNet, 1, classPatternList, 1, U"Draw", nullptr, 0, nullptr);
	praat_addAction2 (classFFNet, 1, classPatternList, 1, U"Draw activation...", nullptr, 0, DO_FFNet_PatternList_drawActivation);
	praat_addAction2 (classFFNet, 1, classPatternList, 1, U"Analyse", nullptr, 0, nullptr);
	praat_addAction2 (classFFNet, 1, classPatternList, 1, U"To Categories...", nullptr, 0, DO_FFNet_PatternList_to_Categories);
	praat_addAction2 (classFFNet, 1, classPatternList, 1, U"To ActivationList...", nullptr, 0, DO_FFNet_PatternList_to_ActivationList);

	praat_addAction2 (classFFNet, 1, classPCA, 1, U"Draw decision plane...", nullptr, 0, DO_FFNet_PCA_drawDecisionPlaneInEigenspace);
	
	praat_addAction2 (classRBM, 1, classPatternList, 1, U"To ActivationList", nullptr, 0, DO_RBM_PatternList_to_ActivationList);

	praat_addAction2 (classPatternList, 1, classCategories, 1, U"To FFNet...", nullptr, 0, DO_PatternList_Categories_to_FFNet);

	praat_addAction3 (classFFNet, 1, classPatternList, 1, classActivationList, 1, U"Get total costs...", nullptr, 0, DO_FFNet_PatternList_ActivationList_getCosts_total);
	praat_addAction3 (classFFNet, 1, classPatternList, 1, classActivationList, 1, U"Get average costs...", nullptr, 0, DO_FFNet_PatternList_ActivationList_getCosts_average);
	praat_addAction3 (classFFNet, 1, classPatternList, 1, classActivationList, 1, U"Learn", nullptr, 0, nullptr);
	praat_addAction3 (classFFNet, 1, classPatternList, 1, classActivationList, 1, U"Learn...", nullptr, 0, DO_FFNet_PatternList_ActivationList_learnSM);
	praat_addAction3 (classFFNet, 1, classPatternList, 1, classActivationList, 1, U"Learn slow...", nullptr, 0, DO_FFNet_PatternList_ActivationList_learnSD);

	praat_addAction3 (classFFNet, 1, classPatternList, 1, classCategories, 1, U"Get total costs...", nullptr, 0, DO_FFNet_PatternList_Categories_getCosts_total);
	praat_addAction3 (classFFNet, 1, classPatternList, 1, classCategories, 1, U"Get average costs...", nullptr, 0, DO_FFNet_PatternList_Categories_getCosts_average);
	praat_addAction3 (classFFNet, 1, classPatternList, 1, classCategories, 1, U"Learn", nullptr, 0, nullptr);
	praat_addAction3 (classFFNet, 1, classPatternList, 1, classCategories, 1, U"Learn...", nullptr, 0, DO_FFNet_PatternList_Categories_learnSM);
	praat_addAction3 (classFFNet, 1, classPatternList, 1, classCategories, 1, U"Learn slow...", nullptr, 0, DO_FFNet_PatternList_Categories_learnSD);

	INCLUDE_MANPAGES (manual_FFNet_init)
}

/* End of file praat_FFnet_init.cpp */
