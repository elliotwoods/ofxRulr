#pragma once

#include "Serializable.h"
#include "ofxCvGui/Element.h"
#include "ofxCvGui/InspectController.h"

#include "ofxRulr/Utils/Constants.h"

namespace ofxRulr {
	namespace Utils {
		class OFXRULR_API_ENTRY AbstractCaptureSet : public Serializable {
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

				ofxLiquidEvent<void> onChange;

				ofxLiquidEvent<void> onDeletePressed;
				ofxLiquidEvent<bool> onSelectionChanged;
				ofParameter<chrono::system_clock::time_point> timestamp{ "Timestamp", chrono::system_clock::now() };
				void rebuildDateStrings();

				const string& getDateString() const;
				const string& getTimeString() const;
				const string& getSecondString() const;
			protected:
				ofParameter<bool> selected{ "Selected", true };
				virtual ofxCvGui::ElementPtr getDataDisplay();
				void callbackSelectedChanged(bool &);

				string timeString;
				string secondString;
				string dateString;
			};

			AbstractCaptureSet();
			virtual ~AbstractCaptureSet();

			void add(shared_ptr<BaseCapture>);
			void remove(shared_ptr<BaseCapture>);
			void clear();
			void resize(size_t count, std::function<shared_ptr<BaseCapture>()> createFunction);

			void select(shared_ptr<BaseCapture>);
			void selectAll();
			void selectNone();
			void deleteSelection();

			void populateWidgets(shared_ptr<ofxCvGui::Panels::Widgets>);
			void serialize(nlohmann::json &);
			void deserialize(const nlohmann::json &);

			vector<shared_ptr<BaseCapture>> getSelectionUntyped() const;
			vector<shared_ptr<BaseCapture>> getAllCapturesUntyped() const;

			virtual shared_ptr<BaseCapture> makeEmpty() const = 0;

			ofxLiquidEvent<void> onChange;
			ofxLiquidEvent<void> onSelectionChanged;

			size_t size() const;
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
				auto newEntry = make_shared<CaptureType>();
				return newEntry;
			}

			virtual std::string getTypeName() const override
			{
				return "CaptureSet";
			}

			void sortByDate()
			{
				this->sortBy([](shared_ptr<CaptureType> capture) {
					auto millis = std::chrono::time_point_cast<std::chrono::milliseconds>(capture->timestamp.get());
					return (float)millis.time_since_epoch().count();
				});
			}

			void sortBy(function<float(shared_ptr<CaptureType>)> sortFunction) {
				sort(this->captures.begin(), this->captures.end()
					, [&](const shared_ptr<BaseCapture>& a, const shared_ptr<BaseCapture>& b) {
						auto aTyped = dynamic_pointer_cast<CaptureType>(a);
						auto bTyped = dynamic_pointer_cast<CaptureType>(b);

						if (aTyped && bTyped) {
							return sortFunction(aTyped) < sortFunction(bTyped);
						}
						else {
							return false;
						}
					});
				this->onChange.notifyListeners();
				this->viewDirty = true;
			}

		protected:
			bool getIsMultipleSelectionAllowed() override {
				return AllowMultipleSelection;
			}
		};
	}
}