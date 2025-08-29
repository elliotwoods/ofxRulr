#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Nodes/IHasVertices.h"

#include "ofxRulr/Models/Reworld/Module.h"
#include "ofxRulr/Models/Reworld/AxisAngles.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class Installation;
		}
	}

	namespace Data {
		namespace Reworld {
			class Column;

			typedef int ModuleIndex;

			class Module
				: public Utils::AbstractCaptureSet::BaseCapture
				, ofxCvGui::IInspectable
			{
			public:
				typedef Models::Reworld::AxisAngles<float> AxisAngles;

				Module();

				string getTypeName() const override;
				string getDisplayString() const override;

				void drawWorldAdvanced(DrawWorldAdvancedArgs&);
				void drawObjectAdvanced(DrawWorldAdvancedArgs&);

				void setParent(Column*);
				ofxRulr::Nodes::Reworld::Installation* getInstallation() const;

				void init();
				void update();
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);

				glm::mat4 getBulkTransform() const; // major position from arrangement
				glm::mat4 getTransformOffset() const; // minor adjustment from calibration
				glm::mat4 getAbsoluteTransform() const;
				glm::vec3 getPosition() const;

				Models::Reworld::Module<float> getModel() const;
				Models::Reworld::AxisAngles<float> getCurrentAxisAngles() const;
				void setTargetAxisAngles(const AxisAngles&);

				bool needsSendAxisAngles() const;
				const AxisAngles& getAxisAnglesForSend(); // also means we will presume that these values have been sent

				struct : ofParameterGroup {
					ofParameter<int> ID{ "ID", 1 };

					struct : ofParameterGroup {
						ofParameter<float> A{ "A", 0, -1, 1 };
						ofParameter<float> B{ "B", 0, -1, 1 };
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

				struct {
					bool needsSend = true;
					AxisAngles lastSentValues;
				} transmission;
			protected:
				ofxCvGui::ElementPtr getDataDisplay() override;
				Column * parent = nullptr;
			};
		}
	}
}