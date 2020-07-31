/** @file SkylineBinPack.h
	@author Jukka Jyl�nki

	@brief Implements different bin packer algorithms that use the SKYLINE data structure.

	This work is released to Public Domain, do whatever you want with it.
*/
#pragma once

#include <vector>
#include <algorithm>
#include <utility>
#include <iostream>
#include <limits>
#include <cassert>
#include <cstring>
#include <cmath>

#include "Rect.hpp"
#include "GuillotineBinPack.hpp"

namespace rbp {

	using namespace std;

	/// Represents a single level (a horizontal line) of the skyline/horizon/envelope.
	struct SkylineNode {
		/// The starting x-coordinate (leftmost).
		int x;
		/// The y-coordinate of the skyline level line.
		int y;
		/// The line width. The ending coordinate (inclusive) will be x+width-1.
		int width;
	};

	/// Implements bin packing algorithms that use the SKYLINE data structure to store the bin contents.
	///	Uses GuillotineBinPack as the waste map.
	class SkylineBinPack {
	public:
		/// Instantiates a bin of size (0,0). Call Init to create a new bin.
		SkylineBinPack() : binWidth(0), binHeight(0) {}

		/// Instantiates a bin of the given size.
		SkylineBinPack(int width, int height, bool useWasteMap_) { Init(width, height, useWasteMap_); }

		/// (����)��ʼ��packer
		void Init(int width, int height, bool useWasteMap_) {
			binWidth = width;
			binHeight = height;
			useWasteMap = useWasteMap_;
			if (useWasteMap) {
				wasteMap.Init(binWidth, binHeight);
				wasteMap.GetFreeRectangles().clear();
			}
			usedSurfaceArea = 0;
			skyLine.clear();
			skyLine.push_back({ 0, 0, binWidth });
			debug_run(disjointRects.Clear());
		}

		/// ��������ʽ�������.
		enum LevelChoiceHeuristic {
			LevelBottomLeft,   // ��С�߶�
			LevelMinWasteFit   // ��С�˷�
		};

		/// ��һ�����η��õ�bin�У����ο�����ת
		/// @param rects Դ�����б�
		/// @param dst Ŀ�ľ����б�
		/// @param method �������.
		void Insert(std::vector<RectSize> &rects, std::vector<Rect> &dst, LevelChoiceHeuristic method) {
			dst.clear();
			while (!rects.empty()) { // ÿ�ε������÷���һ������
				Rect bestNode;
				int bestScore1 = std::numeric_limits<int>::max();
				int bestScore2 = std::numeric_limits<int>::max();
				int bestSkylineIndex = -1;
				int bestRectIndex = -1;

				for (int i = 0; i < rects.size(); ++i) {
					Rect newNode;
					int score1, score2, skylineIndex;
					switch (method) {
					case LevelChoiceHeuristic::LevelBottomLeft:
						newNode = FindPositionForNewNodeBottomLeft(rects[i].width, rects[i].height, score1, score2, skylineIndex);
						debug_assert(disjointRects.Disjoint(newNode));
						break;
					case LevelChoiceHeuristic::LevelMinWasteFit:
						newNode = FindPositionForNewNodeMinWaste(rects[i].width, rects[i].height, score1, score2, skylineIndex);
						debug_assert(disjointRects.Disjoint(newNode));
						break;
					default:
						assert(false);
						break;
					}
					if (newNode.height != 0) {
						if (score1 < bestScore1 || (score1 == bestScore1 && score2 < bestScore2)) {
							bestNode = newNode;
							bestScore1 = score1;
							bestScore2 = score2;
							bestSkylineIndex = skylineIndex;
							bestRectIndex = i;
						}
					}
				}

				// û�о����ܷ���
				if (bestRectIndex == -1) return;

				// ִ�з���
				debug_assert(disjointRects.Disjoint(bestNode));
				debug_run(disjointRects.Add(bestNode));
				AddSkylineLevel(bestSkylineIndex, bestNode); // ����skyline
				usedSurfaceArea += rects[bestRectIndex].width * rects[bestRectIndex].height;
				rects.erase(rects.begin() + bestRectIndex);
				dst.push_back(bestNode);
			}
		}

		/// ���������η��õ�bin�У����ο�����ת
		Rect Insert(int width, int height, LevelChoiceHeuristic method) {
			// ������waste map�г��Է���
			Rect node = wasteMap.Insert(width, height, true, GuillotineBinPack::RectBestShortSideFit, GuillotineBinPack::SplitMaximizeArea);

			// waste map�п��Է���
			if (node.height != 0) {
				debug_assert(disjointRects.Disjoint(node));
				debug_run(disjointRects.Add(node));
				usedSurfaceArea += width * height; // �������skyline
				return node;
			}

			// waste map�в��ܷ���
			switch (method) {
			case LevelChoiceHeuristic::LevelBottomLeft:
				return InsertMinHeight(width, height);
			case LevelChoiceHeuristic::LevelMinWasteFit:
				return InsertMinWaste(width, height);
			default:
				assert(false);
				return node;
			}
		}

		/// ����������.
		float Occupancy() const { return (float)usedSurfaceArea / (binWidth * binHeight); }


	protected:
		/// ������С�߶ȣ�����skylineѡ�����λ��
		Rect FindPositionForNewNodeBottomLeft(int width, int height, int &bestHeight, int &bestWidth, int &bestIndex) const {
			bestHeight = std::numeric_limits<int>::max();
			bestWidth = std::numeric_limits<int>::max(); // �߶���ͬѡ��skyline��Ƚ�С��
			bestIndex = -1;
			Rect newNode;
			memset(&newNode, 0, sizeof(newNode));
			for (int i = 0; i < skyLine.size(); ++i) {
				int y; // ���η��������꣬���ô���
				for (int rotate = 0; rotate <= 1; ++rotate) {
					if (rotate) { swap(width, height); } // ��ת
					if (RectangleFits(i, width, height, y)) {
						if (y + height < bestHeight || (y + height == bestHeight && skyLine[i].width < bestWidth)) {
							bestHeight = y + height;
							bestWidth = skyLine[i].width;
							bestIndex = i;
							newNode.x = skyLine[i].x;
							newNode.y = y;
							newNode.width = width;
							newNode.height = height;
							debug_assert(disjointRects.Disjoint(newNode));
						}
					}
				}
			}
			return newNode;
		}

		/// ���ڡ���С�˷ѡ�������skylineѡ�����λ��
		Rect FindPositionForNewNodeMinWaste(int width, int height, int &bestWastedArea, int &bestHeight, int &bestIndex) const {
			bestWastedArea = std::numeric_limits<int>::max();
			bestHeight = std::numeric_limits<int>::max(); // �˷������ͬѡ��߶Ƚ�С��
			bestIndex = -1;
			Rect newNode;
			memset(&newNode, 0, sizeof(newNode));
			for (int i = 0; i < skyLine.size(); ++i) {
				int y, wastedArea; // ���ô���
				for (int rotate = 0; rotate <= 1; ++rotate) {
					if (rotate) { swap(width, height); } // ��ת
					if (RectangleFits(i, width, height, y, wastedArea)) {
						if (wastedArea < bestWastedArea || (wastedArea == bestWastedArea && y + height < bestHeight)) {
							bestWastedArea = wastedArea;
							bestHeight = y + height;
							bestIndex = i;
							newNode.x = skyLine[i].x;
							newNode.y = y;
							newNode.width = width;
							newNode.height = height;
							debug_assert(disjointRects.Disjoint(newNode));
						}
					}
				}
			}
			return newNode;
		}

		/// �жϾ����ܷ��ڵ�ǰskylineIndex�����ã����ؾ��η��õ�������y
		bool RectangleFits(int skylineIndex, int width, int height, int &y) const {
			int i = skylineIndex;
			int x = skyLine[i].x;
			if (x + width > binWidth) { return false; }
			y = skyLine[i].y;
			int widthLeft = width;
			while (widthLeft > 0) {
				y = max(y, skyLine[i].y); // ��rect.width > skyline.width����Ҫ���y����
				if (y + height > binHeight) { return false; }
				widthLeft -= skyLine[i].width;
				++i;
				assert(i < skyLine.size() || widthLeft <= 0);
			}
			return true;
		}

		/// ͬ�ϣ�ͬʱ���ط���������ֲ���
		bool RectangleFits(int skylineIndex, int width, int height, int &y, int &wastedArea) const {
			bool fits = RectangleFits(skylineIndex, width, height, y);
			if (fits) { wastedArea = ComputeWastedArea(skylineIndex, width, height, y); }
			return fits;
		}

		/// �������������ֲ���
		int ComputeWastedArea(int skylineIndex, int width, int height, int y) const {
			int wastedArea = 0;
			const int rectLeft = skyLine[skylineIndex].x;
			const int rectRight = rectLeft + width;
			for (int i = skylineIndex; i < skyLine.size() && skyLine[i].x < rectRight; ++i) {
				if (skyLine[i].x >= rectRight || skyLine[i].x + skyLine[i].width <= rectLeft) { break; }

				int leftSide = skyLine[i].x;
				int rightSide = min(rectRight, skyLine[i].x + skyLine[i].width);
				assert(y >= skyLine[i].y);
				wastedArea += (rightSide - leftSide) * (y - skyLine[i].y);
			}
			return wastedArea;
		}

		/// ����skyline����֧�ֿ�skyline������
		void AddSkylineLevel(int skylineIndex, const Rect &rect) {
			if (useWasteMap) { AddWasteMapArea(skylineIndex, rect.width, rect.height, rect.y); }

			SkylineNode newNode{ rect.x, rect.y + rect.height, rect.width };
			assert(newNode.x + newNode.width <= binWidth);
			assert(newNode.y <= binHeight);
			skyLine.insert(skyLine.begin() + skylineIndex, newNode);

			for (int i = skylineIndex + 1; i < skyLine.size(); ++i) {
				assert(skyLine[i - 1].x <= skyLine[i].x);
				if (skyLine[i].x < skyLine[i - 1].x + skyLine[i - 1].width) {
					int shrink = skyLine[i - 1].x + skyLine[i - 1].width - skyLine[i].x;
					skyLine[i].x += shrink;
					skyLine[i].width -= shrink;

					if (skyLine[i].width <= 0) {
						skyLine.erase(skyLine.begin() + i);
						--i;
					}
					else { break; }
				}
				else { break; }
			}
			MergeSkylines();
		}

		/// ����waste map
		void AddWasteMapArea(int skylineIndex, int width, int height, int y) {
			const int rectLeft = skyLine[skylineIndex].x;
			const int rectRight = rectLeft + width;
			for (int i = skylineIndex; i < skyLine.size() && skyLine[i].x < rectRight; ++i) {
				if (skyLine[i].x >= rectRight || skyLine[i].x + skyLine[i].width <= rectLeft) { break; }

				int leftSide = skyLine[i].x;
				int rightSide = min(rectRight, leftSide + skyLine[i].width);
				assert(y >= skyLine[i].y);
				Rect waste{ leftSide, skyLine[i].y, rightSide - leftSide, y - skyLine[i].y };
				debug_assert(disjointRects.Disjoint(waste));
				wasteMap.GetFreeRectangles().push_back(waste);
			}
		}

		/// �ϲ�ͬһlevel��skyline�ڵ�.
		void MergeSkylines() {
			for (int i = 0; i < skyLine.size() - 1; ++i) {
				if (skyLine[i].y == skyLine[i + 1].y) {
					skyLine[i].width += skyLine[i + 1].width;
					skyLine.erase(skyLine.begin() + i + 1);
					--i;
				}
			}
		}

		Rect InsertMinHeight(int width, int height) {
			int bestHeight, bestWidth, bestIndex;
			Rect newNode = FindPositionForNewNodeBottomLeft(width, height, bestHeight, bestWidth, bestIndex);
			if (bestIndex != -1) {
				debug_assert(disjointRects.Disjoint(newNode));
				debug_run(disjointRects.Add(newNode));
				AddSkylineLevel(bestIndex, newNode);
				usedSurfaceArea += width * height;
			}
			else {
				memset(&newNode, 0, sizeof(newNode));
			}
			return newNode;
		}

		Rect InsertMinWaste(int width, int height) {
			int bestWastedArea, bestHeight, bestIndex;
			Rect newNode = FindPositionForNewNodeMinWaste(width, height, bestWastedArea, bestHeight, bestIndex);
			if (bestIndex != -1) {
				debug_assert(disjointRects.Disjoint(newNode));
				debug_run(disjointRects.Add(newNode));
				AddSkylineLevel(bestIndex, newNode);
				usedSurfaceArea += width * height;
			}
			else {
				memset(&newNode, 0, sizeof(newNode));
			}
			return newNode;
		}


	protected:
		int binWidth;
		int binHeight;

		std::vector<SkylineNode> skyLine;

		/// ��ʹ�����
		unsigned long usedSurfaceArea;

		/// If true, we use the GuillotineBinPack structure to recover wasted areas into a waste map.
		bool useWasteMap;
		GuillotineBinPack wasteMap;

		debug_run(DisjointRectCollection disjointRects;);
	};

}