//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#include <list>
#include <random>
#include <numeric>

#include "Instance.hpp"
#include "SkylineBinPack.hpp"
#include "QAPCluster.hpp"

namespace fbp {

	using namespace rbp;

	class FloorplanBinPack final : public SkylineBinPack {

	public:
		FloorplanBinPack() = delete;

		FloorplanBinPack(const Instance &ins, const QAPCluster &cluster,
			int bin_width, int bin_height, int group_num = 1, bool use_waste_map = false) :
			_ins(ins), _cluster(cluster), _src(_ins.get_rects()), _min_bin_height(numeric_limits<int>::max()), _group_num(group_num),
			SkylineBinPack(bin_width, bin_height, use_waste_map) {

			// set random seed
			_seed = random_device{}();

			init_group_info();
			init_sort_rules();
		}

		/// ������������
		enum LevelGroupSearch {
			LevelSelfishly,       // ����ǰ����
			LevelNeighborAll,     // ��ǰ�����ȫ���ھӷ���
			LevelNeighborPartial  // [todo] ��ǰ�������/���ھӷ��飬��ɿ��ǰ��ٷֱ�ѡȡһ���־���
		};

		/// ���ò���
		enum LevelHeuristicSearch {
			LevelMinHeightFit,    // ��С�߶�rectangle�������ǿ�skyline�Ҳ����
			LevelMinWasteFit,     // ��С�˷�rectangle�������ǿ�skyline�Ҳ����
			LevelBottomLeftScore  // ��������skyline�����ǿ�skyline�Ҳ����
		};

		/// ����binWidth��������ֲ�������������С�߶�
		int random_local_search(int iter) {
			// first time to call RLS
			if (iter == 1) {
				for (auto &rule : _sort_rules) {
					_rects.assign(rule.sequence.begin(), rule.sequence.end());
					vector<Rect> target_dst;
					target_dst.reserve(_rects.size());
					rule.target_height = insert_bottom_left_score(target_dst, LevelGroupSearch::LevelSelfishly);
					if (rule.target_height < _min_bin_height) {
						_min_bin_height = rule.target_height;
						_dst = target_dst;
					}
				}
				sort(_sort_rules.begin(), _sort_rules.end(), [](auto &lhs, auto &rhs) { return lhs.target_height > rhs.target_height; });
			}

			// �����ʼ��
			static default_random_engine gen(_seed);
			vector<int> probs;
			probs.reserve(_sort_rules.size());
			for (int i = 1; i <= _sort_rules.size(); ++i) { probs.push_back(2 * i); }
			discrete_distribution<> discrete_dist(probs.begin(), probs.end());
			auto &picked_rule = _sort_rules[discrete_dist(gen)];
			_rects.assign(picked_rule.sequence.begin(), picked_rule.sequence.end());
			vector<Rect> target_dst;
			target_dst.reserve(_rects.size());
			picked_rule.target_height = min(picked_rule.target_height, insert_bottom_left_score(target_dst, LevelGroupSearch::LevelSelfishly));
			if (picked_rule.target_height < _min_bin_height) {
				_min_bin_height = picked_rule.target_height;
				_dst = target_dst;
			}

			// �����Ż�
			for (int i = 1; i <= iter; ++i) {
				SortRule new_rule = picked_rule;
				uniform_int_distribution<> uniform_dist(0, new_rule.sequence.size() - 1);
				int a = uniform_dist(gen);
				int b = uniform_dist(gen);
				while (a == b) { b = uniform_dist(gen); }
				swap(new_rule.sequence[a], new_rule.sequence[b]);
				_rects.assign(new_rule.sequence.begin(), new_rule.sequence.end());
				vector<Rect> target_dst;
				target_dst.reserve(_rects.size());
				new_rule.target_height = min(new_rule.target_height, insert_bottom_left_score(target_dst, LevelGroupSearch::LevelSelfishly));
				if (new_rule.target_height < picked_rule.target_height) {
					picked_rule = new_rule;
				}
				if (picked_rule.target_height < _min_bin_height) {
					_min_bin_height = picked_rule.target_height;
					_dst = target_dst;
				}
			}

			// ������������б�
			sort(_sort_rules.begin(), _sort_rules.end(), [](auto &lhs, auto &rhs) { return lhs.target_height > rhs.target_height; });

			return _min_bin_height;
		}

		int insert_bottom_left_score(vector<Rect> &dst, LevelGroupSearch method_1) {
			int min_bin_height = 0;
			dst.clear();
			while (!_rects.empty()) {
				auto bottom_skyline_iter = min_element(skyLine.begin(), skyLine.end(),
					[](auto &lhs, auto &rhs) { return lhs.y < rhs.y; });
				int best_skyline_index = distance(skyLine.begin(), bottom_skyline_iter);
				list<int> candidate_rects = get_candidate_rects(*bottom_skyline_iter, method_1);
				auto min_width_iter = min_element(candidate_rects.begin(), candidate_rects.end(),
					[this](int lhs, int rhs) { return _src.at(lhs).width < _src.at(rhs).width; });
				auto min_height_iter = min_element(candidate_rects.begin(), candidate_rects.end(),
					[this](int lhs, int rhs) { return _src.at(lhs).height < _src.at(rhs).height; });
				int min_width = min(_src.at(*min_width_iter).width, _src.at(*min_height_iter).height);

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
					if (best_rect_index == -1) { return binHeight; }

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
					usedSurfaceArea += _src.at(best_rect_index).width * _src.at(best_rect_index).height;
					_rects.remove(best_rect_index);
					dst.push_back(best_node);
					min_bin_height = max(min_bin_height, best_node.y + best_node.height);
				}
			}

			return min_bin_height;
		}

		int insert_greedy_fit(vector<Rect> &dst, LevelGroupSearch method_1, LevelHeuristicSearch method_2) {
			int min_bin_height = 0;
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
					list<int> candidate_rects = get_candidate_rects(skyLine[i], method_1);
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
				if (best_rect_index == -1) { return binHeight; }

				// ִ�з���
				debug_assert(disjointRects.Disjoint(best_node));
				debug_run(disjointRects.Add(best_node));
				AddSkylineLevel(best_skyline_index, best_node);
				usedSurfaceArea += _src.at(best_rect_index).width * _src.at(best_rect_index).height;
				_rects.remove(best_rect_index);
				dst.push_back(best_node);
				min_bin_height = max(min_bin_height, best_node.y + best_node.height);
			}

			return min_bin_height;
		}

	private:
		/// ��ʼ��������Ϣ
		void init_group_info() {
			if (_group_num == 1) {
				_group_boundaries.resize(1, { 0, 0, 1.0*binWidth, 1.0*binHeight });
				_group_neighbors.resize(1, { false });
				for (auto &rect : _src) { rect.gid = 0; }
			}
			else {
				_group_num = _cluster.qap_sol.size();
				double unit_width = 1.0*binWidth / sqrt(_group_num);
				double unit_height = 1.0*binHeight / sqrt(_group_num);
				_group_boundaries.resize(_group_num);
				for (int gid = 0; gid < _group_num; ++gid) {
					_group_boundaries[gid].x = unit_width * _cluster.distance_nodes.at(gid).first;
					_group_boundaries[gid].y = unit_height * _cluster.distance_nodes.at(gid).second;
					_group_boundaries[gid].width = unit_width;
					_group_boundaries[gid].height = unit_height;
				}
				_group_neighbors.resize(_group_num, vector<bool>(_group_num, false));
				for (int gi = 0; gi < _group_num; ++gi) {
					for (int gj = gi + 1; gj < _group_num; ++gj) {
						int cbsv_dis = QAPCluster::cal_distance(QAPCluster::LevelDistance::ChebyshevDis,
							_cluster.distance_nodes.at(gi).first, _cluster.distance_nodes.at(gi).second,
							_cluster.distance_nodes.at(gj).first, _cluster.distance_nodes.at(gj).second);
						if (cbsv_dis == 1) { // �б�ѩ�����Ϊ1����Ϊ�ھ�
							_group_neighbors[gi][gj] = true;
							_group_neighbors[gj][gi] = true;
						}
					}
				}
				for (auto &rect : _src) { rect.gid = _cluster.qap_sol.at(_cluster.part.at(rect.id)); }
			}
		}

		/// ��ʼ����������б�
		void init_sort_rules() {
			vector<int> seq(_src.size());
			// 0_����˳��
			iota(seq.begin(), seq.end(), 0);
			for (int i = 0; i < 5; ++i) { _sort_rules.push_back({ seq, numeric_limits<int>::max() }); }
			// 1_����ݼ�
			sort(_sort_rules[1].sequence.begin(), _sort_rules[1].sequence.end(), [this](int lhs, int rhs) {
				return (_src.at(lhs).width * _src.at(lhs).height) > (_src.at(rhs).width * _src.at(rhs).height); });
			// 2_�߶ȵݼ�
			sort(_sort_rules[2].sequence.begin(), _sort_rules[2].sequence.end(), [this](int lhs, int rhs) {
				return _src.at(lhs).height > _src.at(rhs).height; });
			// 3_��ȵݼ�
			sort(_sort_rules[3].sequence.begin(), _sort_rules[3].sequence.end(), [this](int lhs, int rhs) {
				return _src.at(lhs).width > _src.at(rhs).width; });
			// 4_�������
			shuffle(_sort_rules[4].sequence.begin(), _sort_rules[4].sequence.end(), mt19937{ _seed });

			// Ĭ�ϸ���_rect����˳�����ڲ���̰���㷨
			_rects.assign(_sort_rules[0].sequence.begin(), _sort_rules[0].sequence.end());
		}

		/// ���ڷ��������ѡ��ѡ���Σ���С������ģ
		list<int> get_candidate_rects(const SkylineNode &skyline, LevelGroupSearch method) {
			int gid = 0;
			for (; gid < _group_num; ++gid) {
				if (skyline.x >= _group_boundaries[gid].x && skyline.y >= _group_boundaries[gid].y
					&& skyline.x < _group_boundaries[gid].x + _group_boundaries[gid].width
					&& skyline.y < _group_boundaries[gid].y + _group_boundaries[gid].height) {
					break;
				}
			}
			list<int> candidate_rects;
			switch (method) {
			case LevelGroupSearch::LevelSelfishly:
				for (int r : _rects) { // ����˳�����_rects����ʹ��sort_rule��Ч
					if (_src.at(r).gid == gid) { candidate_rects.push_back(r); }
				}
				break;
			case LevelGroupSearch::LevelNeighborAll:
				for (int r : _rects) {
					if (_src.at(r).gid == gid) { candidate_rects.push_back(r); }
					else if (_group_neighbors[_src.at(r).gid][gid]) {
						candidate_rects.push_back(r);
					}
				}
				break;
			case LevelGroupSearch::LevelNeighborPartial:
				for (int r : _rects) {
					if (_src.at(r).gid == gid) { candidate_rects.push_back(r); }
					else if (_group_neighbors[_src.at(r).gid][gid] && _src.at(r).gid < gid) {
						candidate_rects.push_back(r);
					}
				}
				break;
			default:
				assert(false);
				break;
			}
			// ���û�з��ϲ��Եľ��Σ��򷵻����о���
			if (candidate_rects.empty()) { candidate_rects = _rects; }
			return candidate_rects;
		}

		/// ����idea����������/����ʹ�ֲ���
		Rect find_rect_for_skyline_bottom_left(int skyline_index, const list<int> &rects, int &best_rect) {
			int best_score = -1;
			Rect new_node;
			memset(&new_node, 0, sizeof(new_node)); // ȷ���޽�ʱ���ظ߶�Ϊ0
			for (int r : rects) {
				int x, score;
				for (int rotate = 0; rotate <= 1; ++rotate) {
					int width = _src.at(r).width, height = _src.at(r).height;
					if (rotate) { swap(width, height); }
					if (score_rect_for_skyline_bottom_left(skyline_index, width, height, x, score)) {
						if (best_score < score) {
							best_score = score;
							best_rect = r;
							new_node = { _src.at(r).id, _src.at(r).gid, x, skyLine[skyline_index].y, width, height };
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

		/// ������С�߶ȣ�Ϊ��ǰskylineѡ����ѷ��õľ���
		Rect find_rect_for_skyline_min_height(int skyline_index, const list<int> &rects,
			int &best_height, int &best_width, int &best_rect) {
			best_height = numeric_limits<int>::max();
			best_width = numeric_limits<int>::max(); // �߶���ͬѡ���Ƚ�С�ģ�[todo]����ѡ���Ƚϴ�ıȽϺã�
			best_rect = -1;
			Rect new_node;
			memset(&new_node, 0, sizeof(new_node));
			for (int r : rects) {
				int y;
				for (int rotate = 0; rotate <= 1; ++rotate) {
					int width = _src.at(r).width, height = _src.at(r).height;
					if (rotate) { swap(width, height); }
					if (RectangleFits(skyline_index, width, height, y)) {
						if (y + height < best_height || (y + height == best_height && width < best_width)) {
							best_height = y + height;
							best_width = width;
							best_rect = r;
							new_node = { _src.at(r).id, _src.at(r).gid, skyLine[skyline_index].x, y, width, height };
							debug_assert(disjointRects.Disjoint(new_node));
						}
					}
				}
			}
			return new_node;
		}

		/// ������С�˷�
		Rect find_rect_for_skyline_min_waste(int skyline_index, const list<int> &rects,
			int &best_wasted_area, int &best_height, int &best_rect) {
			best_wasted_area = numeric_limits<int>::max();
			best_height = numeric_limits<int>::max(); // ���������ͬѡ��߶�С��
			best_rect = -1;
			Rect new_node;
			memset(&new_node, 0, sizeof(new_node));
			for (int r : rects) {
				int y, wasted_area;
				for (int rotate = 0; rotate <= 1; ++rotate) {
					int width = _src.at(r).width, height = _src.at(r).height;
					if (rotate) { swap(width, height); }
					if (RectangleFits(skyline_index, width, height, y, wasted_area)) {
						if (wasted_area < best_wasted_area || (wasted_area == best_wasted_area && y + height < best_height)) {
							best_wasted_area = wasted_area;
							best_height = y + height;
							best_rect = r;
							new_node = { _src.at(r).id, _src.at(r).gid, skyLine[skyline_index].x, y, width, height };
							debug_assert(disjointRects.Disjoint(new_node));
						}
					}
				}
			}
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
		const Instance &_ins;
		const QAPCluster &_cluster;

		vector<Rect> _src; // Դ�����б���ʼ�����޸�
		vector<Rect> _dst;
		int _min_bin_height;

		/// ���������
		struct SortRule {
			vector<int> sequence;
			int target_height;
		};
		vector<SortRule> _sort_rules; // ��������б���������ֲ�����
		list<int> _rects;			  // SortRule��sequence���൱��ָ�룬ʹ��list����ɾ�����������Ϊ��

		/// ������Ϣ
		int _group_num;
		struct Boundary {
			double x, y;
			double width, height;
		};
		vector<Boundary> _group_boundaries;    // `_group_boundaries[i]`   ����_i�ı߽�
		vector<vector<bool>> _group_neighbors; // `_group_neighbors[i][j]` ����_i��_j�Ƿ�Ϊ�ھ�

		unsigned int _seed;
	};
}

