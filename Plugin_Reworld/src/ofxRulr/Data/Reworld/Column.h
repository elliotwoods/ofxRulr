#pragma once

#include "Module.h"
#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class Installation;
		}
	}

	namespace Data {
		namespace Reworld {
			typedef int ColumnIndex;

			class Column
				: public Utils::AbstractCaptureSet::BaseCapture
				, ofxCvGui::IInspectable
			{
			public:
				Column();
				string getTypeName() const override;
				string getDisplayString() const override;

				void drawWorldAdvanced(DrawWorldAdvancedArgs&);
				void drawObjectAdvanced(DrawWorldAdvancedArgs&);

				void setParent(Nodes::Reworld::Installation*);
				void init();
				void update();
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);
				void build(int columnIndex, int countY, float pitch);

				glm::mat4 getAbsoluteTransform() const;

				vector<shared_ptr<Module>> getAllModules() const;
				shared_ptr<Module> getModuleByIndex(ModuleIndex, bool selectedOnly) const;
				map<ModuleIndex, shared_ptr<Module>> getSelectedModulesByIndex() const;

				vector<string> getAndClearOSCOutbox();

				struct : ofParameterGroup {
					ofParameter<int> index{ "Index", 0 };
					PARAM_DECLARE("Column", index);
				} parameters;

				Nodes::Reworld::Installation* parent;
				shared_ptr<Nodes::Item::RigidBody> rigidBody;

				Utils::CaptureSet<Module> modules;

				Utils::EditSelection<Module> ourSelection;
				Utils::EditSelection<Column>* parentSelection = nullptr;

			protected:
				ofxCvGui::ElementPtr getDataDisplay() override;
				vector<string> oscOutbox;
			};
		}
	}
}