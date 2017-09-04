#pragma once

#include "ofxNonLinearFit.h"
#include "AddView.h"
#include "AddScan.h"

#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/Projector.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				namespace ProCamSolver {
					class Model : public ofxNonLinearFit::Models::Base<AddScan::DataPoint, Model> {
					public:
						Model(shared_ptr<AddView> camera, shared_ptr<AddView> projector);

						unsigned int getParameterCount() const override;
						double getResidual(AddScan::DataPoint dataPoint) const override;
						void evaluate(AddScan::DataPoint &) const override;
						void cacheModel() override;


						virtual void resetParameters() override;
						
						vector<double> getLowerBounds() const;
						vector<double> getUpperBounds() const;

					protected:
						shared_ptr<Item::Camera> camera;
						shared_ptr<Item::Projector> projector;
						vector<FitParameter *> fitParameters;

						ofxRay::Camera cameraCached;
						ofxRay::Projector projectorCached;
					};
				}
			}
		}
	}
}