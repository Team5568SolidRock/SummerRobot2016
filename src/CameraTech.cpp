/*
 * Camera.cpp
 *
 *  Created on: Sep 1, 2016
 *      Author: Alex's
 */
#include "WPILib.h"
#include <vector>
#include <cmath>


class CameraTech
{
	struct ParticleReport {
			double PercentAreaToImageArea;
			double Area;
			double ConvexHullArea;
			double BoundingRectLeft;
			double BoundingRectTop;
			double BoundingRectRight;
			double BoundingRectBottom;
		};

	struct Scores {
			double Trapezoid;
			double LongAspect;
			double ShortAspect;
			double AreaToConvexHullArea;
		};
		Range Hue_Range = {0, 0};	//Default hue range for yellow tote
		Range Sat_Range = {0, 0};	//Default saturation range for yellow tote
		Range Val_Range = {0, 0};	//Default value range for yellow tote
		double AREA_MINIMUM = 2; //Default Area minimum for particle as a percentage of total image area
		double LONG_RATIO = 2.22; //Tote long side = 26.9 / Tote height = 12.1 = 2.22
		double SHORT_RATIO = 1.4; //Tote short side = 16.9 / Tote height = 12.1 = 1.4
		double SCORE_MIN = 75.0;  //Minimum score to be considered a tote
		double VIEW_ANGLE = 49.4; //View angle fo camera, set to Axis m1011 by default, 64 for m1013, 51.7 for 206, 52 for HD3000 square, 60 for HD3000 640x480
		ParticleFilterCriteria2 criteria[1];
		ParticleFilterOptions2 filterOptions = {0,0,1,1};
		Scores scores;
		int imaqError;
		Image *binaryFrame;

		void ColorSelect (Range Hue , Range Saturation , Range Value)
		{
			Hue_Range = Hue;
			Sat_Range = Saturation;
			Val_Range = Value;

		}




			//Comparator function for sorting particles. Returns true if particle 1 is larger
			static bool CompareParticleSizes(ParticleReport particle1, ParticleReport particle2)
			{
				//we want descending sort order
				return particle1.PercentAreaToImageArea > particle2.PercentAreaToImageArea;
			}

			/**
			 * Converts a ratio with ideal value of 1 to a score. The resulting function is piecewise
			 * linear going from (0,0) to (1,100) to (2,0) and is 0 for all inputs outside the range 0-2
			 */
			double ratioToScore(double ratio)
			{
				return (fmax(0, fmin(100*(1-fabs(1-ratio)), 100)));
			}

			/**
			 * Method to score convex hull area. This scores how "complete" the particle is. Particles with large holes will score worse than a filled in shape
			 */
			double ConvexHullAreaScore(ParticleReport report)
			{
				return ratioToScore((report.Area/report.ConvexHullArea)*1.18);
			}

			/**
			 * Method to score if the particle appears to be a trapezoid. Compares the convex hull (filled in) area to the area of the bounding box.
			 * The expectation is that the convex hull area is about 95.4% of the bounding box area for an ideal tote.
			 */
			double TrapezoidScore(ParticleReport report)
			{
				return ratioToScore(report.ConvexHullArea/((report.BoundingRectRight-report.BoundingRectLeft)*(report.BoundingRectBottom-report.BoundingRectTop)*.954));
			}

			/**
			 * Method to score if the aspect ratio of the particle appears to match the long side of a tote.
			 */
			double LongSideScore(ParticleReport report)
			{
				return ratioToScore(((report.BoundingRectRight-report.BoundingRectLeft)/(report.BoundingRectBottom-report.BoundingRectTop))/LONG_RATIO);
			}

			/**
			 * Method to score if the aspect ratio of the particle appears to match the short side of a tote.
			 */
			double ShortSideScore(ParticleReport report){
				return ratioToScore(((report.BoundingRectRight-report.BoundingRectLeft)/(report.BoundingRectBottom-report.BoundingRectTop))/SHORT_RATIO);
			}

			/**
			 * Computes the estimated distance to a target using the width of the particle in the image. For more information and graphics
			 * showing the math behind this approach see the Vision Processing section of the ScreenStepsLive documentation.
			 *
			 * @param image The image to use for measuring the particle estimated rectangle
			 * @param report The Particle Analysis Report for the particle
			 * @param isLong Boolean indicating if the target is believed to be the long side of a tote
			 * @return The estimated distance to the target in feet.
			 */
			double computeDistance (Image *image, ParticleReport report, bool isLong) {
				double normalizedWidth, targetWidth;
				int xRes, yRes;

				imaqGetImageSize(image, &xRes, &yRes);
				normalizedWidth = 2*(report.BoundingRectRight - report.BoundingRectLeft)/xRes;
				SmartDashboard::PutNumber("Width", normalizedWidth);
				targetWidth = isLong ? 26.9 : 16.9;

				return  targetWidth/(normalizedWidth*12*tan(VIEW_ANGLE*M_PI/(180*2)));
			}

			void SendToDashboard(Image *image, int error)
									{
										if(error < ERR_SUCCESS) {
											//DriverStation::ReportError("Send To Dashboard error: " + std::to_string((long)imaqError) + "\n");
										} else {
											CameraServer::GetInstance()->SetImage(image);
										}
									}
public:

			CameraTech()
	{

	}
			/* void LineDetect (IMAQdxSession Cam , Image*frame)
			{
				 Point point = {1,1};
				 PixelValu PixelValue
				 int test;
				 test = imaqGetPixel(frame,point,Pixelvalue);
			}*/

			void ColorPickUp(IMAQdxSession Cam , Image*frame , float AreaMin , Range Hue, Range Sat, Range Val)
			{
							ColorSelect(Hue, Sat, Val);
							binaryFrame = imaqCreateImage(IMAQ_IMAGE_U8, 0);
							imaqError = IMAQdxOpenCamera("cam0", IMAQdxCameraControlModeController, &Cam);



							//Threshold the image looking for Color
							imaqError = imaqColorThreshold(binaryFrame, frame, 255, IMAQ_HSV, &Hue_Range, &Sat_Range, &Val_Range);

							//Set numParticles
						//	int numParticles = 0;
							//imaqError = imaqCountParticles(binaryFrame, 1, &numParticles);


							//Send masked image to dashboard to assist in tweaking mask.
							//Puts color image on Dashboard
							SendToDashboard(binaryFrame, imaqError);

							//filter out small particles
							//float areaMin = SmartDashboard::GetNumber("Area min %", AREA_MINIMUM);
							/*float areaMin = AreaMin;
							criteria[0] = {IMAQ_MT_AREA_BY_IMAGE_AREA, areaMin, 100, false, false};
							imaqError = imaqParticleFilter4(binaryFrame, binaryFrame, criteria, 1, &filterOptions, NULL, NULL);


							imaqError = imaqCountParticles(binaryFrame, 1, &numParticles);*/




							Wait(0.005);				// wait for a motor update time
						}



};



