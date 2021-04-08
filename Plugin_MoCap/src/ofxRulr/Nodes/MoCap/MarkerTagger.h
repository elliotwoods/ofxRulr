#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Utils/CaptureSet.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			class MarkerTagger : public Nodes::Base {
			public:
				class Capture : public Utils::AbstractCaptureSet::BaseCapture {
				public:
					Capture();
					string getDisplayString() const override;
					void serialize(nlohmann::json &);
					void deserialize(const nlohmann::json &);

					void tagMarker(const glm::vec2 & imageCoordinate);
					void removeMarker(const glm::vec2 & imageCoordinate);
					void drawOnImage();

					cv::Mat image;
					cv::Mat blurred;
					cv::Mat difference;
					cv::Mat binary;
					vector<vector<cv::Point2i>> contours;
					vector<vector<cv::Point2i>> filteredContours;
					vector<cv::Rect> boundingBoxes;

					chrono::nanoseconds timestamp;
					map<int, cv::Point2f> markers;
				};

				MarkerTagger();
				string getTypeName() const override;
				void init();
				void update();
				void serialize(nlohmann::json &);
				void deserialize(const nlohmann::json &);
				void populateInspector(ofxCvGui::InspectArguments &);

				ofxCvGui::PanelPtr getPanel() override;
			protected:
				shared_ptr<Capture> getSelection();

				MAKE_ENUM(PreviewType
					, (Raw, Blurred, Diff, Binary)
					, ("Raw", "Blur", "Diff", "Binary"));

				void addCapture();

				void updatePreview();
				void updateCount();

				void callbackPreviewMode(PreviewType &);

				shared_ptr<ofxCvGui::Panels::Image> panel;
				ofImage preview;

				Utils::CaptureSet<Capture, false> captures;

				map<int, size_t> count;

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<float> blurSize{ "Blur size", 100, 0, 1000 };
						ofParameter<float> threshold{ "Threshold", 50, 0, 255 };
						ofParameter<float> differenceAmplify{ "Difference amplify", 4, 1, 16 };
						PARAM_DECLARE("Local difference", blurSize, threshold, differenceAmplify);
					} localDifference;

					struct : ofParameterGroup {
						ofParameter<float> minimumArea{ "Minimum area sqrt [px]", 10, 0, 1000 };
						ofParameter<float> maximumArea{ "Maximum area sqrt [px]", 200, 0, 1000 };
						PARAM_DECLARE("Contour filter", minimumArea, maximumArea);
					} contourFilter;

					ofParameter<PreviewType> previewType{ "Preview type", PreviewType::Binary };

					PARAM_DECLARE("MarkerTagger", localDifference, contourFilter, previewType);
				} parameters;
			};
		}
	}
}