#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Nodes/IHasVertices.h"

#include "ofxRulr/Models/Reworld/Module.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class Installation;
		}
	}

	namespace Data {
		namespace Reworld {
			class Column;

			class Module
				: public Utils::AbstractCaptureSet::BaseCapture
				, ofxCvGui::IInspectable
			{
			public:
				Module();

				string getTypeName() const override;
				string getDisplayString() const override;

				void drawWorldAdvanced(DrawWorldAdvancedArgs&);
				void drawObjectAdvanced(DrawWorldAdvancedArgs&);

				void setParent(Column*);
				ofxRulr::Nodes::Reworld::Installation* getInstallation() const;

				void init();
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);

				glm::mat4 getBulkTransform() const; // major position from arrangement
				glm::mat4 getTransformOffset() const; // minor adjustment from calibration
				glm::mat4 getAbsoluteTransform() const;
				Models::Reworld::Module<float> getModel() const;
				Models::Reworld::Module<float>::AxisAngles getCurrentAxisAngles() const;
				void setTargetAxisAngles(const Models::Reworld::Module<float>::AxisAngles&);

				struct : ofParameterGroup {
					ofParameter<int> ID{ "ID", 1 };

					struct : ofParameterGroup {
						ofParameter<float> A{ "A", 0, -1, 1 };
						ofParameter<float> B{ "A", 0, -1, 1 };
						PARAM_DECLARE("Axis angles", A, B);
					} axisAngles;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							struct : ofParameterGroup {
								ofParameter<float> x{ "X", 0, -1, 1 };
								ofParameter<float> y{ "Y", 0, -1, 1 };
								ofParameter<float> z{ "Z", 0, -1, 1 };
								PARAM_DECLARE("Translation", x, y, z);
							} translation;

							struct : ofParameterGroup {
								ofParameter<float> x{ "X", 0, -TWO_PI, TWO_PI };
								ofParameter<float> y{ "Y", 0, -TWO_PI, TWO_PI };
								ofParameter<float> z{ "Z", 0, -TWO_PI, TWO_PI };
								PARAM_DECLARE("Rotation vector", x, y, z);
							} rotationVector;

							PARAM_DECLARE("Transform offset", translation, rotationVector);
						} transformOffset;

						struct : ofParameterGroup {
							ofParameter<float> A{ "A", 0, -1, 1 };
							ofParameter<float> B{ "A", 0, -1, 1 };
							PARAM_DECLARE("Axis angle offsets", A, B);
						} axisAngleOffsets;

						PARAM_DECLARE("Calibration parameters", transformOffset, axisAngleOffsets);
					} calibrationParameters;
			

					PARAM_DECLARE("Module", ID, axisAngles, calibrationParameters);
				} parameters;

				shared_ptr<Nodes::Item::RigidBody> positionInColumn;

				Utils::EditSelection<Module>* parentSelection = nullptr;

			protected:
				ofxCvGui::ElementPtr getDataDisplay() override;
				Column * parent = nullptr;
			};
		}
	}
}