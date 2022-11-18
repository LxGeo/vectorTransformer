#pragma once
#include "defs.h"

namespace LxGeo
{
	namespace vectorTransformer
	{

		/**
		*  A vectorTransformerApp class to manage running required steps to generate final transformed vector.
		*/
		class vectorTransformerApp
		{

		public:
			vectorTransformerApp() {};

			~vectorTransformerApp() {};

			/**
			*  A method used to run all steps of transformation.
			*/
			virtual void run();

			/**
			*  A method to check requirements before running transformation steps.
			* Example: -Checking vector exsitance, checking output_path overwrite, check algorithm parameters ...
			* @return an bool indicating if can run algorithm
			*/
			bool pre_check();

		};
	}
}