#pragma once

#include "Base.h"

namespace ofxRulr{
	namespace Nodes {
		namespace Item {
			class AbstractBoard  : public Base {
			public:
				MAKE_ENUM(FindBoardMode
					, (Raw, Optimized, Assistant)
					, ("Raw", "Optimized", "Assistant"));

				AbstractBoard();
				virtual string getTypeName() const override;

				virtual bool findBoard(cv::Mat, vector<cv::Point2f> & result, vector<cv::Point3f> & objectPoints, FindBoardMode findBoardMode, cv::Mat cameraMatrix, cv::Mat distortionCoefficients) const;

				template<typename FilteredPointTypeA, typename FilteredPointTypeB>
				static void filterCommonPoints(vector<FilteredPointTypeA> & imagePointsA
					, vector<FilteredPointTypeB> & imagePointsB
					, vector <cv::Point3f> & objectPointsA
					, vector <cv::Point3f> & objectPointsB) {

					if (imagePointsA.size() != objectPointsA.size()) {
						throw(ofxRulr::Exception("filterCommonPoints : Size mismatch error for set A"));
					}
					if (imagePointsB.size() != objectPointsB.size()) {
						throw(ofxRulr::Exception("filterCommonPoints : Size mismatch error for set B"));
					}

					vector<FilteredPointTypeA> imagePointsFilteredA;
					vector<FilteredPointTypeB> imagePointsFilteredB;
					vector<cv::Point3f> objectPointsFiltered;
					{
						for (int i = 0; i < imagePointsA.size(); i++) {
							for (int j = 0; j < imagePointsB.size(); j++) {
								if (objectPointsA[i] == objectPointsB[j]) {
									imagePointsFilteredA.push_back(imagePointsA[i]);
									imagePointsFilteredB.push_back(imagePointsB[j]);
									objectPointsFiltered.push_back(objectPointsA[i]);
								}
							}
						}
					}

					imagePointsA = imagePointsFilteredA;
					imagePointsB = imagePointsFilteredB;
					objectPointsA = objectPointsFiltered;
					objectPointsB = objectPointsFiltered;
				}
			protected:
			};
		}
	}
}