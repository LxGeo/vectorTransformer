#include "io_raster.h"
#include "io_shapefile.h"
#include "defs.h"
#include "parameters.h"
#include "vector_transformer_app.h"
#include "stitching/raster_stitcher.h"
#include "ops.h"
#include "tqdm/tqdm.h"
#include "graph_weights/polygons_spatial_weights.h"


namespace LxGeo
{

	using namespace IO_DATA;
	namespace vectorTransformer
	{

		bool vectorTransformerApp::pre_check() {

			std::cout << "Pre check input parameters" << std::endl;

			PolygonsShapfileIO target_shape;
			RasterIO ref_raster;
			bool target_loaded = target_shape.load_shapefile(params->input_shapefile_to_transform, true);
			if (!target_loaded) {
				std::cout << "Error loading shapefile to align at: " << params->input_shapefile_to_transform << std::endl;
				return false;
			}
			bool ref_loaded = ref_raster.load_raster(params->input_ref_raster);
			if (!ref_loaded) {
				std::cout << "Error loading reference raster at: " << params->input_ref_raster << std::endl;
				return false;
			}

			if (!target_shape.spatial_refrence->IsSame(ref_raster.spatial_refrence)) {
				std::cout << "Input shapefiles have different spatial reference system!" << std::endl;
				return false;
			}


			//output dirs creation
			boost::filesystem::path output_path(params->output_basename);
			boost::filesystem::path output_parent_dirname = output_path.parent_path();
			boost::filesystem::path output_temp_path = output_parent_dirname / params->temp_dir;
			params->temp_dir = output_temp_path.string();
			if (!boost::filesystem::exists(output_parent_dirname))
			{
				//boost::filesystem::create_directory(output_parent_dirname);
				//std::cout << fmt::format("Directory Created: {}", output_parent_dirname.string()) << std::endl;
			}

			if (!boost::filesystem::exists(output_temp_path))
			{
				boost::filesystem::create_directory(output_temp_path);
				std::cout << fmt::format("Directory Created: {}", output_temp_path.string()) << std::endl;
			}

			if (boost::filesystem::exists(output_path) && !params->overwrite_output) {
				std::cout << fmt::format("output shapefile already exists: {}!", output_path.string()) << std::endl;
				std::cout << fmt::format("Add --overwrite_output !", output_path.string()) << std::endl;
				return false;
			}

			return true;

		}

		void vectorTransformerApp::run() {

			PolygonsShapfileIO target_shape;
			RasterIO ref_raster;
			bool target_loaded = target_shape.load_shapefile(params->input_shapefile_to_transform, false);
			bool ref_loaded = ref_raster.load_raster(params->input_ref_raster, GA_ReadOnly, false);
			if (!target_loaded | !ref_loaded)
				return;
			// get common AOI extents
			OGREnvelope target_envelope, ref_envelope;
			target_shape.vector_layer->GetExtent(&target_envelope); 
			ref_envelope = transform_B2OGR_Envelope(ref_raster.bounding_box);
			OGREnvelope union_envelope(target_envelope); union_envelope.Merge(ref_envelope);

			using pixel_type = cv::Vec2f;

			RasterPixelsStitcher<pixel_type> rps(ref_raster);
			auto stitch_option = ElementaryStitchOptions(ElementaryStitchStrategy::spatial_corner_area, pix_buff_t(), { 4,4,4,4 });

			MedianAggregator<pixel_type> agg;
			//std::vector<Elementary_Pinned_Pixels_Boost_Polygon_2<pixel_type>> elementary_pinned_geometry_container;
			std::vector<Boost_Polygon_2> elementary_pinned_geometry_container;
			elementary_pinned_geometry_container.reserve(target_shape.geometries_container.size());

			PolygonSpatialWeights PSW = PolygonSpatialWeights(target_shape.geometries_container);
			WeightsDistanceBandParams wdbp = { 15, false, -1, [](double x)->double { return x; } };
			PSW.fill_distance_band_graph(wdbp);
			tqdm bar;

			/*for (size_t geom_idx = 0; geom_idx < target_shape.geometries_container.size(); geom_idx++) {
				bar.progress(geom_idx, target_shape.geometries_container.size());
				Elementary_Pinned_Pixels_Boost_Polygon_2<pixel_type> c_pinned_poly;
				boost::geometry::assign(c_pinned_poly, target_shape.geometries_container[geom_idx]);
				rps.readElementaryPixels(c_pinned_poly, stitch_option);
				boost::geometry::for_each_point(
					c_pinned_poly,
					self_translator<Elementary_Pinned_Pixels_Boost_Point_2<pixel_type>, pixel_type>(agg, ref_raster.geotransform[1], ref_raster.geotransform[5])
				);
				// fix last non transformed point in polygon rings
				boost::geometry::assign(*c_pinned_poly.outer().rbegin(), *c_pinned_poly.outer().begin());
				for (auto& c_inner_ring: c_pinned_poly.inners())
					boost::geometry::assign(*c_inner_ring.rbegin(), *c_inner_ring.begin());
				elementary_pinned_geometry_container.push_back(c_pinned_poly);
			}*/

			// rigid transform
			for (size_t geom_idx = 0; geom_idx < target_shape.geometries_container.size(); geom_idx++) {
				bar.progress(geom_idx, target_shape.geometries_container.size());
				Structural_Pinned_Pixels_Boost_Polygon_2<pixel_type> c_pinned_poly;
				int num_interior_rings = target_shape.geometries_container[geom_idx].inners().size();
				if (num_interior_rings>0)
					c_pinned_poly.inners().resize(num_interior_rings); //temp because assign doesn't do it
				boost::geometry::assign(c_pinned_poly, target_shape.geometries_container[geom_idx]);
				rps.readStructrualPixels(c_pinned_poly);
				cv::Scalar disp = agg(c_pinned_poly.outer_pinned_pixel);
				disp[0] *= ref_raster.geotransform[1];
				disp[1] *= ref_raster.geotransform[5];
				bg::strategy::transform::translate_transformer<double, 2, 2> trans_obj(disp[0], disp[1]);
				elementary_pinned_geometry_container.push_back(translate_geometry(target_shape.geometries_container[geom_idx],trans_obj));
			}


			bar.finish();

			std::vector<Polygon_with_attributes> temp_output_container; temp_output_container.reserve(elementary_pinned_geometry_container.size());
			for (const auto& in_p : elementary_pinned_geometry_container) {
				Boost_Polygon_2 out_p;
				boost::geometry::assign(out_p, in_p);
				temp_output_container.push_back(Polygon_with_attributes(out_p));
			}
			
			PolygonsShapfileIO out_shape(params->output_shapefile, target_shape.spatial_refrence);
			out_shape.write_polygon_shapefile(temp_output_container);
			std::cout << target_shape.geometries_container.size() << std::endl;
		}

	}
}