#pragma once
#include "defs.h"
#include "aggregators/aggregator.h"


namespace LxGeo {
	namespace vectorTransformer {

        template <typename point, typename pixel_values_type>
        class self_translator
        {
        public:
            typedef GeometryFactoryShared::Aggregator<pixel_values_type> agg_t;
        private:
            agg_t* pixel_values_aggreagtor;
            double x_scale, y_scale;

        public:
            self_translator(agg_t& agg, double _x_scale, double _y_scale): pixel_values_aggreagtor(&agg), x_scale(_x_scale), y_scale(_y_scale){}

            inline void operator()(point& p)
            {
                auto res = pixel_values_aggreagtor->operator()(p.pinned_pixel);
                double x_t = res[0], y_t = res[1];                

                using boost::geometry::get;
                using boost::geometry::set;
                set<0>(p, get<0>(p) + x_scale*x_t);
                set<1>(p, get<1>(p) + y_scale*y_t);
            }
        };

	}
}