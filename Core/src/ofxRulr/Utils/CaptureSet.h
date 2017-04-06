#pragma once

#include "Serializable.h"
#include "ofxCvGui/Element.h"
#include "ofxCvGui/InspectController.h"

#include "ofxRulr/Utils/Constants.h"

namespace ofxRulr {
	namespace Utils {
		class RULR_EXPORTS AbstractCaptureSet : public Serializable {
		public:
			class BaseCapture : public Serializable {
			public:
				BaseCapture();
				ofxCvGui::ElementPtr getGuiElement();
				virtual string getDisplayString() const = 0;
				std::string getTypeName() const override;

				bool isSelected() const;
				void setSelected(bool);

				ofParameter<ofColor> color{ "Color", ofColor() };

				ofxLiquidEvent<void> onDeletePressed;
				ofxLiquidEvent<bool> onSelectionChanged;
			protected:
				ofParameter<bool> selected{ "Selected", true };
				virtual ofxCvGui::ElementPtr getDataDisplay();
				void callbackSelectedChanged(bool &);

				void rebuildDateStrings();
				ofParameter<chrono::system_clock::time_point> timestamp{ "Timestamp", chrono::system_clock::now() };
				string timeString;
				string secondString;
				string dateString;
			};

			AbstractCaptureSet();
			virtual ~AbstractCaptureSet();

			void add(shared_ptr<BaseCapture>);
			void remove(shared_ptr<BaseCapture>);
			void clear();

			void select(shared_ptr<BaseCapture>);
			void selectAll();
			void selectNone();

			void populateWidgets(shared_ptr<ofxCvGui::Panels::Widgets>);
			void serialize(Json::Value &);
			void deserialize(const Json::Value &);

			vector<shared_ptr<BaseCapture>> getSelectionUntyped() const;
			vector<shared_ptr<BaseCapture>> getAllCapturesUntyped() const;

			virtual shared_ptr<BaseCapture> makeEmpty() const = 0;

			ofxLiquidEvent<void> onSelectionChanged;
		protected:
			virtual bool getIsMultipleSelectionAllowed() = 0;
			vector<shared_ptr<BaseCapture>> captures;

			shared_ptr<ofxCvGui::Panels::Widgets> listView;

			bool viewDirty = true;
		};

		template<typename CaptureType, bool AllowMultipleSelection = true>
		class CaptureSet : public AbstractCaptureSet {
		public:
			vector<shared_ptr<CaptureType>> getVectorTyped(vector<shared_ptr<BaseCapture>> vectorUntyped) const {
				vector<shared_ptr<CaptureType>> vectorTyped;

				for (auto captureUntyped : vectorUntyped) {
					auto capture = dynamic_pointer_cast<CaptureType>(captureUntyped);
					vectorTyped.push_back(capture);
				}
				return vectorTyped;
			}

			vector<shared_ptr<CaptureType>> getSelection() const {
				return getVectorTyped(getSelectionUntyped());
			}

			vector<shared_ptr<CaptureType>> getAllCaptures() const {
				return getVectorTyped(getAllCapturesUntyped());
			}

			shared_ptr<BaseCapture> makeEmpty() const override {
				return make_shared<CaptureType>();
			}

			virtual std::string getTypeName() const override
			{
				return "CaptureSet";
			}
		protected:
			bool getIsMultipleSelectionAllowed() override {
				return AllowMultipleSelection;
			}
		};
	}
}