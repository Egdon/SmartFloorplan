//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#include "SkylineBinPack.hpp"

namespace fbp {

	using namespace rbp;

	class FloorplanBinPack final : public SkylineBinPack {

	public:
		FloorplanBinPack() = delete;

		FloorplanBinPack(vector<Rect> &rects, int bin_width, int bin_height, bool use_waste_map = false, int group_num = 1) :
			_rects(rects), SkylineBinPack(bin_width, bin_height, use_waste_map) {
			init_rects_groups(group_num);
		}

		/// ������������
		enum LevelGroupSearch {
			LevelSelfishly,      // ����ǰ����
			LevelNeighborAll,    // ��ǰ�����ȫ���ھӷ���
			LevelNeighborPartial // [todo] ��ǰ����Ͳ����ھӷ��飬�򰴰ٷֱ�ѡȡ�ھӵĲ��־���
		};

		/// ���ò���
		enum LevelHeuristicSearch {
			LevelMinHeightFit,    // ��С�߶�rectangle�������ǿ�skyline�Ҳ����
			LevelMinWasteFit,     // ��С�˷�rectangle�������ǿ�skyline�Ҳ����
			LevelBottomLeftScore  // ��������skyline�����ǿ�skyline�Ҳ����
		};

		void insert_greedy_fit(vector<Rect> &dst, LevelGroupSearch method_1, LevelHeuristicSearch method_2) {
			dst.clear();
			while (!_rects.empty()) {
				Rect best_node;
				int best_score_1 = numeric_limits<int>::max();
				int best_score_2 = numeric_limits<int>::max();
				int best_skyline_index = -1;
				int best_rect_index = -1;

				for (int i = 0; i < skyLine.size(); ++i) {
					Rect new_node;
					int score_1, score_2, rect_index;
					vector<Rect> candidate_rects = get_candidate_rects(skyLine[i], method_1);
					switch (method_2) {
					case LevelHeuristicSearch::LevelMinHeightFit:
						new_node = find_rect_for_skyline_min_height(i, candidate_rects, score_1, score_2, rect_index);
						debug_assert(disjointRects.Disjoint(new_node));
						break;
					case LevelHeuristicSearch::LevelMinWasteFit:
						new_node = find_rect_for_skyline_min_waste(i, candidate_rects, score_1, score_2, rect_index);
						debug_assert(disjointRects.Disjoint(new_node));
						break;
					default:
						assert(false);
						break;
					}
					if (new_node.height != 0) {
						if (score_1 < best_score_1 || (score_1 == best_score_1 && score_2 < best_score_2)) {
							best_node = new_node;
							best_score_1 = score_1;
							best_score_2 = score_2;
							best_skyline_index = i;
							best_rect_index = rect_index;
						}
					}
				}

				// û�о����ܷ���
				if (best_rect_index == -1) { return; }

				// ִ�з���
				debug_assert(disjointRects.Disjoint(best_node));
				debug_run(disjointRects.Add(best_node));
				AddSkylineLevel(best_skyline_index, best_node);
				usedSurfaceArea += _rects[best_rect_index].width * _rects[best_rect_index].height;
				// [todo] ɾ�������еľ��Σ���Ϊָ�룿
				_group_rects[_rects[best_rect_index].group_id].erase(find_if(
					_group_rects[_rects[best_rect_index].group_id].begin(),
					_group_rects[_rects[best_rect_index].group_id].end(),
					[this, best_rect_index](Rect &rect) { return rect.id == _rects[best_rect_index].id; }
				));
				_rects.erase(_rects.begin() + best_rect_index);
				dst.push_back(best_node);
			}
		}

		void insert_bottom_left_score(vector<Rect> &dst, LevelGroupSearch method_1) {
			dst.clear();
			while (!_rects.empty()) {
				auto bottom_skyline_iter = min_element(skyLine.begin(), skyLine.end(),
					[](auto &lhs, auto &rhs) { return lhs.y < rhs.y; });
				int best_skyline_index = distance(skyLine.begin(), bottom_skyline_iter);
				vector<Rect> candidate_rects = get_candidate_rects(*bottom_skyline_iter, method_1);
				auto minwidth_rect_iter = min_element(candidate_rects.begin(), candidate_rects.end(),
					[](auto &lhs, auto &rhs) { return lhs.width < rhs.width; });
				auto minheight_rect_iter = min_element(candidate_rects.begin(), candidate_rects.end(),
					[](auto &lhs, auto &rhs) { return lhs.height < rhs.height; });
				int min_width = min(minwidth_rect_iter->width, minheight_rect_iter->height);

				if (bottom_skyline_iter->width < min_width) { // ��С��Ȳ������㣬��Ҫ���
					if (best_skyline_index == 0) { skyLine[best_skyline_index].y = skyLine[best_skyline_index + 1].y; }
					else if (best_skyline_index == skyLine.size() - 1) { skyLine[best_skyline_index].y = skyLine[best_skyline_index - 1].y; }
					else { skyLine[best_skyline_index].y = min(skyLine[best_skyline_index - 1].y, skyLine[best_skyline_index + 1].y); }
					MergeSkylines();
				}
				else { 
					int best_rect_index = -1;
					Rect best_node = find_rect_for_skyline_bottom_left(best_skyline_index, candidate_rects, best_rect_index);

					// û�о����ܷ���
					if (best_rect_index == -1) { return; }

					// ִ�з���
					debug_assert(disjointRects.Disjoint(best_node));
					debug_run(disjointRects.Add(best_node));
					if (best_node.x == skyLine[best_skyline_index].x) { // ����
						AddSkylineLevel(best_skyline_index, best_node);
					}
					else { // ����
						SkylineNode new_skyline{ best_node.x, best_node.y + best_node.height, best_node.width };
						assert(new_skyline.x + new_skyline.width <= binWidth);
						assert(new_skyline.y <= binHeight);
						skyLine.insert(skyLine.begin() + best_skyline_index + 1, new_skyline);
						skyLine[best_skyline_index].width -= best_node.width;
						MergeSkylines();
					}
					usedSurfaceArea += _rects[best_rect_index].width * _rects[best_rect_index].height;
					// [todo] ɾ�������еľ��Σ���Ϊָ�룿
					_group_rects[_rects[best_rect_index].group_id].erase(find_if(
						_group_rects[_rects[best_rect_index].group_id].begin(),
						_group_rects[_rects[best_rect_index].group_id].end(),
						[this, best_rect_index](Rect &rect) { return rect.id == _rects[best_rect_index].id; }
					));
					_rects.erase(_rects.begin() + best_rect_index);
					dst.push_back(best_node);
				}
			}
		}

	private:
		/// ����QAP�����η��飬��ʼ��������Ϣ
		void init_rects_groups(int group_num) {
			_group_rects.resize(group_num);
			// [todo] 
			_group_rects[0] = _rects;
		}

		/// ���ڷ��������ѡ��ѡ���Σ���С������ģ
		vector<Rect> get_candidate_rects(const SkylineNode &skyline, LevelGroupSearch method) {
			int group_id = 0;
			for (; group_id < _group_boundaries.size(); ++group_id) {
				if (skyline.x >= _group_boundaries[group_id].x && skyline.y >= _group_boundaries[group_id].y
					&& skyline.x < _group_boundaries[group_id].x + _group_boundaries[group_id].width
					&& skyline.y < _group_boundaries[group_id].y + _group_boundaries[group_id].height) {
					break;
				}
			}
			vector<Rect> candidate_rects(_group_rects[group_id]);
			switch (method) {
			case LevelGroupSearch::LevelNeighborAll:
				for (int id : _group_neighbors[group_id]) {
					candidate_rects.insert(candidate_rects.end(), _group_rects[id].begin(), _group_rects[id].end());
				}
				break;
			case LevelGroupSearch::LevelNeighborPartial:
				for (int id : _group_neighbors[group_id]) {
					if (id < group_id) { // [todo] ��������/�µķ���
						candidate_rects.insert(candidate_rects.end(), _group_rects[id].begin(), _group_rects[id].end());
					}
				}
				break;
			case LevelGroupSearch::LevelSelfishly:
				break;
			default:
				assert(false);
				break;
			}
			return candidate_rects;
		}

		/// ������С�߶ȣ�Ϊ��ǰskylineѡ����ѷ��õľ���
		Rect find_rect_for_skyline_min_height(int skyline_index, const vector<Rect> &rects,
			int &best_height, int &best_width, int &best_index) {
			best_height = numeric_limits<int>::max();
			best_width = numeric_limits<int>::max(); // �߶���ͬѡ���Ƚ�С��,[todo]����ѡ���Ƚϴ�ıȽϺã�
			best_index = -1;
			Rect new_node;
			memset(&new_node, 0, sizeof(new_node)); // ȷ���޽�ʱ���ظ߶�Ϊ0
			for (int i = 0; i < rects.size(); ++i) {
				int y;
				for (int rotate = 0; rotate <= 1; ++rotate) {
					int width = rects[i].width, height = rects[i].height;
					if (rotate) { swap(width, height); }
					if (RectangleFits(skyline_index, width, height, y)) {
						if (y + height < best_height || (y + height == best_height && width < best_width)) {
							best_height = y + height;
							best_width = width;
							best_index = i;
							new_node = { rects[i].id, rects[i].group_id, skyLine[skyline_index].x, y, width, height };
							debug_assert(disjointRects.Disjoint(new_node));
						}
					}
				}
			}
			return new_node;
		}

		/// ������С�˷�
		Rect find_rect_for_skyline_min_waste(int skyline_index, const vector<Rect> &rects,
			int &best_wasted_area, int &best_height, int &best_index) {
			best_wasted_area = numeric_limits<int>::max();
			best_height = numeric_limits<int>::max();
			best_index = -1;
			Rect new_node;
			memset(&new_node, 0, sizeof(new_node));
			for (int i = 0; i < rects.size(); ++i) {
				int y, wasted_area;
				for (int rotate = 0; rotate <= 1; ++rotate) {
					int width = rects[i].width, height = rects[i].height;
					if (rotate) { swap(width, height); }
					if (RectangleFits(skyline_index, width, height, y, wasted_area)) {
						if (wasted_area < best_wasted_area || (wasted_area == best_wasted_area && y + height < best_height)) {
							best_wasted_area = wasted_area;
							best_height = y + height;
							best_index = i;
							new_node = { rects[i].id, rects[i].group_id, skyLine[skyline_index].x, y, width, height };
							debug_assert(disjointRects.Disjoint(new_node));
						}
					}
				}
			}
			return new_node;
		}

		/// ����idea����������/����ʹ�ֲ���
		Rect find_rect_for_skyline_bottom_left(int skyline_index, const vector<Rect> &rects, int &best_index) {
			int best_score = -1;
			Rect new_node;
			memset(&new_node, 0, sizeof(new_node));
			for (int i = 0; i < rects.size(); ++i) {
				int x, score;
				for (int rotate = 0; rotate <= 1; ++rotate) {
					int width = rects[i].width, height = rects[i].height;
					if (rotate) { swap(width, height); }
					if (score_rect_for_skyline_bottom_left(skyline_index, width, height, x, score)) {
						if (best_score < score) {
							best_score = score;
							best_index = i;
							new_node = { rects[i].id, rects[i].group_id, x, skyLine[skyline_index].y, width, height };
							debug_assert(disjointRects.Disjoint(new_node));
						}
					}
				}
			}
			// [todo] δʵ��(d)(f)->(h)���˻����
			if (best_score == 4) {}
			if (best_score == 2) {}
			if (best_score == 0) {}

			return new_node;
		}

		/// space����
		struct SkylineSpace {
			int x;
			int y;
			int width;
			int hl;
			int hr;
		};

		/// ��ֲ���
		bool score_rect_for_skyline_bottom_left(int skyline_index, int width, int height, int &x, int &score) {
			if (width > skyLine[skyline_index].width) { return false; }
			if (skyLine[skyline_index].y + height > binHeight) { return false; }

			SkylineSpace space = skyline_nodo_to_space(skyline_index);
			if (space.hl >= space.hr) {
				if (width == space.width && height == space.hl) { score = 7; }
				else if (width == space.width && height == space.hr) { score = 6; }
				else if (width == space.width && height > space.hl) { score = 5; }
				else if (width < space.width && height == space.hl) { score = 4; }
				else if (width == space.width && height < space.hl && height > space.hr) { score = 3; }
				else if (width < space.width && height == space.hr) { score = 2; } // ����
				else if (width == space.width && height < space.hr) { score = 1; }
				else if (width < space.width && height != space.hl) { score = 0; }
				else { return false; }

				if (score == 2) { x = skyLine[skyline_index].x + skyLine[skyline_index].width - width; }
				else { x = skyLine[skyline_index].x; }
			}
			else { // hl < hr
				if (width == space.width && height == space.hr) { score = 7; }
				else if (width == space.width && height == space.hl) { score = 6; }
				else if (width == space.width && height > space.hr) { score = 5; }
				else if (width < space.width && height == space.hr) { score = 4; } // ����
				else if (width == space.width && height < space.hr && height > space.hl) { score = 3; }
				else if (width < space.width && height == space.hl) { score = 2; }
				else if (width == space.width && height < space.hl) { score = 1; }
				else if (width < space.width && height != space.hr) { score = 0; } // ����
				else { return false; }

				if (score == 4 || score == 0) { x = skyLine[skyline_index].x + skyLine[skyline_index].width - width; }
				else { x = skyLine[skyline_index].x; }
			}
			if (x + width > binWidth) { return false; }

			return true;
		}

		SkylineSpace skyline_nodo_to_space(int skyline_index) {
			int hl, hr;
			if (skyLine.size() == 1) {
				hl = hr = binHeight - skyLine[skyline_index].y;
			}
			else if (skyline_index == 0) {
				hl = binHeight - skyLine[skyline_index].y;
				hr = skyLine[skyline_index + 1].y - skyLine[skyline_index].y;
			}
			else if (skyline_index == skyLine.size() - 1) {
				hl = skyLine[skyline_index - 1].y - skyLine[skyline_index].y;
				hr = binHeight - skyLine[skyline_index].y;
			}
			else {
				hl = skyLine[skyline_index - 1].y - skyLine[skyline_index].y;
				hr = skyLine[skyline_index + 1].y - skyLine[skyline_index].y;
			}
			return { skyLine[skyline_index].x, skyLine[skyline_index].y, skyLine[skyline_index].width, hl, hr };
		}

	private:
		/// ȫ�������þ��Σ��������Ϊ��
		vector<Rect> &_rects;

		/// ������Ϣ
		vector<vector<Rect>> _group_rects;
		vector<vector<int>> _group_neighbors;
		vector<Rect> _group_boundaries;
	};
}




