#pragma once

#include "ofxCeres.h"

namespace ofxRulr{
	namespace Models {
		namespace Reworld {
			template<typename T>
			struct Module {
				// --
				// PARAMETERS BEGIN
				// --
				// 
				
				// The transform from the array of modules
				glm::tmat4x4<T> bulkTransform;

				// The calibrated transform offset
				struct {
					glm::tvec3<T> translation{ (T) 0, (T) 0, (T) 0 };
					glm::tvec3<T> rotationVector{ (T)0, (T)0, (T)0 };
				} transformOffset;
				
				struct {
					T A;
					T B;
				} axisAngleOffsets;
				

				struct {
					T interPrismDistance;
					T prismAngleRadians;
					T ior;
				} installationParameters;

				//
				// --
				// PARAMETERS END
				// --

				template<typename T2>
				Module<T2>
					castTo() const
				{
					Module<T2> castModule;
					castModule.bulkTransform = (glm::tmat4x4<T2>) this->bulkTransform;
					{
						castModule.transformOffset.translation = (glm::tvec3<T2>) this->transformOffset.translation;
						castModule.transformOffset.rotationVector = (glm::tvec3<T2>) this->transformOffset.rotationVector;
					}
					{
						castModule.axisAngleOffsets.A = (T2)this->axisAngleOffsets.A;
						castModule.axisAngleOffsets.B = (T2)this->axisAngleOffsets.B;
					}
					{
						castModule.installationParameters.interPrismDistance = (T2)this->installationParameters.interPrismDistance;
						castModule.installationParameters.prismAngleRadians = (T2)this->installationParameters.prismAngleRadians;
						castModule.installationParameters.ior = (T2)this->installationParameters.ior;
					}
					return castModule;
				}
				
				// The total transform (to the center of the Risley pair)
				glm::tmat4x4<T> getTransform() const {
					return this->bulkTransform
						* ofxCeres::VectorMath::createTransform(this->transformOffset.translation, this->transformOffset.rotationVector);
				}

				glm::tvec3<T> getPosition() const {
					return ofxCeres::VectorMath::applyTransform(this->getTransform(), glm::tvec3<T>(
						(T)0, (T)0, (T)0)
					);
				}

				// Note these are normalised values as per interface
				struct AxisAngles {
					T A;
					T B;
				};

				struct RefractionResult {
					ofxCeres::Models::Ray<T> intermediateRay;
					ofxCeres::Models::Ray<T> outputRay;
				};

				RefractionResult refract(const ofxCeres::Models::Ray<T>& incomingRay
					, const AxisAngles& axisAngles) const
				{
					// This presumes the setup from Shilla Reworld
					// (light from behind the lenses towards the room interior +z to -z)
					
					RefractionResult refractionResult;

					// Get the transform to the center of the body
					auto transform = this->getTransform();

					// Create the planes
					ofxCeres::Models::Plane<T> fresnelPlane1, fresnelPlane2;
					{
						const auto prismZOffset = this->installationParameters.interPrismDistance / (T) 2.0f;

						// The normals represent the flat planes (entrance for 1, exit for 2)
						fresnelPlane1.center = { (T)0, (T)0, (T)prismZOffset };
						fresnelPlane1.normal = { (T)0, (T)0, -1 };
						fresnelPlane1 = fresnelPlane1.transform(transform);

						fresnelPlane2.center = { (T)0, (T)0, (T)-prismZOffset };
						fresnelPlane2.normal = { (T)0, (T)0, -1 };
						fresnelPlane2 = fresnelPlane2.transform(transform);
					}

					auto normalEnterPlane1 = fresnelPlane1.normal;
					auto normalExitPlane2 = fresnelPlane2.normal;

					// Calculate the refraction normals
					glm::tvec3<T> normalExitPlane1, normalEnterPlane2;
					{
						// Untransformed plane normals
						normalExitPlane1 = {
						(T)-sin(this->installationParameters.prismAngleRadians)
						, (T)0.0
						, (T)-cos(this->installationParameters.prismAngleRadians)
						};
						normalEnterPlane2 = {
							(T)sin(this->installationParameters.prismAngleRadians)
							, (T)0.0
							, (T)-cos(this->installationParameters.prismAngleRadians)
						};

						auto axisA = axisAngles.A + axisAngleOffsets.A;
						auto axisB = axisAngles.B + axisAngleOffsets.B;

						// Spin by axis angles
						// (note z axis is inverted and angle is inverted so we use no inversion here)
						normalExitPlane1 = glm::rotateZ(normalExitPlane1, (T) (axisB * TWO_PI));
						normalEnterPlane2 = glm::rotateZ(normalEnterPlane2, (T) (axisA * TWO_PI));

						normalExitPlane1 = ofxCeres::VectorMath::applyRotationOnly(transform, normalExitPlane1);
						normalEnterPlane2 = ofxCeres::VectorMath::applyRotationOnly(transform, normalEnterPlane2);
					}

					auto intersectionAtPlane1 = fresnelPlane1.intersect(incomingRay);
					auto transmissionInsidePlane1 = ofxCeres::VectorMath::refract(incomingRay.t
						, normalEnterPlane1
						, this->installationParameters.ior);

					auto transmissionExitPlane1 = ofxCeres::VectorMath::refract(transmissionInsidePlane1
						, normalExitPlane1
						, (T) 1.0 / this->installationParameters.ior);

					// The ray passing between the planes
					refractionResult.intermediateRay.s = intersectionAtPlane1;
					refractionResult.intermediateRay.t = transmissionExitPlane1;

					// Intersect second plane
					auto intersectionAtPlane2 = fresnelPlane2.intersect(refractionResult.intermediateRay);
					auto transmissionInsidePlane2 = ofxCeres::VectorMath::refract(transmissionExitPlane1
						, normalEnterPlane2
						, this->installationParameters.ior);

					auto transmissionExitPlane2 = ofxCeres::VectorMath::refract(transmissionInsidePlane2
						, normalExitPlane2
						, (T) 1.0 / this->installationParameters.ior);

					refractionResult.outputRay.s = intersectionAtPlane2;
					refractionResult.outputRay.t = transmissionExitPlane2;

					// Truncate the intermediate ray so it intersects plane 2
					refractionResult.intermediateRay.setEnd(intersectionAtPlane2);

					return refractionResult;
				}
			};
		}
	}
}