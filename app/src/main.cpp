
#include "gdal.h"
#include "parameters.h"
#include "GDAL_OPENCV_IO.h"
#include "vector_transformer_app.h"

#include "design_pattern/extended_iterators.h"
#include "stitchable_geometries/def_stitched_geoms.h"

using namespace LxGeo::vectorTransformer;

int main(int argc, char* argv[])
{
	clock_t t_begin = clock();
	//GDALAllRegister();
	KGDAL2CV* kgdal2cv = new KGDAL2CV();

	// Reads command-line parameters
	/*
	std::list<Elementary_Pinned_Pixels_Boost_Point_2<cv::Vec3b>> ll = {{1,1}, {2,2}, {3,3}, {4,4}};
	//auto w = circularIterator<Elementary_Pinned_Pixels_Boost_Point_2<cv::Vec3b>, std::list>(ll);
	Boost_Ring_2 rr;
	bg::append(rr, Boost_Point_2(1, 1));
	bg::append(rr, Boost_Point_2(2, 2));
	bg::append(rr, Boost_Point_2(3, 3));
	bg::append(rr, Boost_Point_2(4, 4));
	
	auto w2 = circularIterator<Boost_Point_2, std::vector>(rr.begin(), std::prev(rr.end()), rr.size()-1);
	auto vals = w2.getWindow(std::next(rr.begin()), 4, 4);
	*/
	params = new Parameters(argc, argv);
	if (!params->initialized()) {
		delete params;
		return 1;
	}

	// Runs process
	vectorTransformerApp v_t_app = vectorTransformerApp();
	if (v_t_app.pre_check())
		v_t_app.run();

	// Quits
	delete kgdal2cv;
	delete params;

	clock_t t_end = clock();
	//std::cout << "** Elapsed time : " << double(t_end - t_begin) / CLOCKS_PER_SEC << " s." << std::endl;

	return 0;
}