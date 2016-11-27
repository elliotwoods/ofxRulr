#pragma once
#include "Node.h"
#include "ofxRulr/Nodes/Item/View.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				namespace ProCamSolver {
					struct FitParameter {
						string name;
						bool enabled;
						float deviation;
						function<float()> get;
						function<void(float)> set;
					};

					class AddView : public Node {
					public:
						AddView();
						string getTypeName() const override;

						void init();

						void serialize(Json::Value &);
						void deserialize(const Json::Value &);
						void populateInspector(ofxCvGui::InspectArguments &);

						vector<FitParameter *> getActiveFitParameters();
					protected:
						shared_ptr<Nodes::Base> getNode() override;

						FitParameter position[3];
						FitParameter rotation[3];
						
						FitParameter throwRatioLog;
						FitParameter pixelAspectRatioLog;
						FitParameter lensOffset[2];

						FitParameter distortion[RULR_VIEW_DISTORTION_COEFFICIENT_COUNT];

						vector<FitParameter *> fitParameters;
					};
				}
			}
		}
	}
}