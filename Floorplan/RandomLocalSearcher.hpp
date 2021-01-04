//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#include "FloorplanPacker.hpp"

namespace fbp {

	using namespace std;

	class RandomLocalSearcher : public FloorplanPacker {

		/// ���������
		struct SortRule {
			vector<int> sequence;
			double target_objective;
		};

	public:

		RandomLocalSearcher() = delete;

		RandomLocalSearcher(const Instance &ins, const vector<Rect> &src, int bin_width, const vector<vector<int>> &graph, default_random_engine &gen) :
			FloorplanPacker(ins, src, bin_width, graph, gen) {
			reset();
			init_sort_rules();
		}

		/// �Ű��ĸ߶��Ͻ磬��`Config::LevelQAPCluster::On`ʹ��
		int get_bin_height() const {
			return max_element(_skyline.begin(), _skyline.end(),
				[](auto &lhs, auto &rhs) { return lhs.y < rhs.y; })->y;
		}

		void set_group_boundaries(const vector<Boundary> &boundaries) { _group_boundaries = boundaries; }

		void set_group_neighbors(const vector<vector<bool>> &neighbors) { _group_neighbors = neighbors; }

		/// ����_bin_width��������ֲ�����
		void run(int iter, double alpha, double beta, Config::LevelWireLength level_wl, Config::LevelObjDist level_dist,
			Config::LevelGroupSearch level_gs = Config::LevelGroupSearch::NoGroup) {
			// the first time to call RLS on W_k
			if (iter == 1) {
				for (auto &rule : _sort_rules) {
					_rects.assign(rule.sequence.begin(), rule.sequence.end());
					vector<Rect> target_dst;
					int target_area = insert_bottom_left_score(target_dst, level_gs) * _bin_width;
					vector<bool> is_packed(_src.size(), true);
					double dist;
					double target_wirelength = cal_wirelength(target_dst, is_packed, dist, level_wl, level_dist);
					rule.target_objective = cal_objective(target_area, dist, alpha, beta);
					update_objective(rule.target_objective, target_area, target_wirelength, target_dst);
				}
				// �������У�Խ�����Ŀ�꺯��ֵԽСѡ�и���Խ��
				sort(_sort_rules.begin(), _sort_rules.end(), [](auto &lhs, auto &rhs) { return lhs.target_objective > rhs.target_objective; });
			}

			// �����Ż�
			SortRule &picked_rule = _sort_rules[_discrete_dist(_gen)];
			for (int i = 1; i <= iter; ++i) {
				SortRule new_rule = picked_rule;
				if (iter % 4) { swap_sort_rule(new_rule); }
				else { rotate_sort_rule(new_rule); }
				_rects.assign(new_rule.sequence.begin(), new_rule.sequence.end());

				vector<Rect> target_dst;
				int target_area = insert_bottom_left_score(target_dst, level_gs) * _bin_width;
				vector<bool> is_packed(_src.size(), true);
				double dist;
				double target_wirelength = cal_wirelength(target_dst, is_packed, dist, level_wl, level_dist);
				new_rule.target_objective = cal_objective(target_area, dist, alpha, beta);
				if (new_rule.target_objective < picked_rule.target_objective) {
					picked_rule = new_rule;
					update_objective(picked_rule.target_objective, target_area, target_wirelength, target_dst);
				}
			}
			// ������������б�
			sort(_sort_rules.begin(), _sort_rules.end(), [](auto &lhs, auto &rhs) { return lhs.target_objective > rhs.target_objective; });
		}

		/// ������������ʹ�ֲ��ԣ�̰�Ĺ���һ��������
		int insert_bottom_left_score(vector<Rect> &dst, Config::LevelGroupSearch method) {
			int skyline_height = 0;
			reset();
			dst = _src;

			while (!_rects.empty()) {
				auto bottom_skyline_iter = min_element(_skyline.begin(), _skyline.end(),
					[](auto &lhs, auto &rhs) { return lhs.y < rhs.y; });
				int best_skyline_index = distance(_skyline.begin(), bottom_skyline_iter);
				list<int> candidate_rects = get_candidate_rects(best_skyline_index, method);
				int min_rect_width = _src.at(*min_element(candidate_rects.begin(), candidate_rects.end(),
					[this](int lhs, int rhs) { return _src.at(lhs).width < _src.at(rhs).width; })).width;

				if (_skyline[best_skyline_index].width < min_rect_width) { // ��С��Ⱦ��ηŲ���ȥ����Ҫ���
					if (best_skyline_index == 0) { _skyline[best_skyline_index].y = _skyline[best_skyline_index + 1].y; }
					else if (best_skyline_index == _skyline.size() - 1) { _skyline[best_skyline_index].y = _skyline[best_skyline_index - 1].y; }
					else { _skyline[best_skyline_index].y = min(_skyline[best_skyline_index - 1].y, _skyline[best_skyline_index + 1].y); }
					merge_skylines(_skyline);
					continue;
				}

				int best_rect_index = find_rect_for_skyline_bottom_left(best_skyline_index, candidate_rects, dst);
				assert(best_rect_index != -1);

				// ��δ�����б���ɾ��
				_rects.remove(best_rect_index);

				// ����skyline
				SkylineNode new_skyline_node{ dst[best_rect_index].x, dst[best_rect_index].y + dst[best_rect_index].height, dst[best_rect_index].width };
				if (new_skyline_node.x == _skyline[best_skyline_index].x) { // ����
					_skyline.insert(_skyline.begin() + best_skyline_index, new_skyline_node);
					_skyline[best_skyline_index + 1].x += new_skyline_node.width;
					_skyline[best_skyline_index + 1].width -= new_skyline_node.width;
					merge_skylines(_skyline);
				}
				else { // ����
					_skyline.insert(_skyline.begin() + best_skyline_index + 1, new_skyline_node);
					_skyline[best_skyline_index].width -= new_skyline_node.width;
					merge_skylines(_skyline);
				}
				skyline_height = max(skyline_height, new_skyline_node.y);
			}

			return skyline_height;
		}

	private:
		/// ÿ�ε�������_skyLine
		void reset() {
			_skyline.clear();
			_skyline.push_back({ 0,0,_bin_width });
		}

		/// ��ʼ����������б�
		void init_sort_rules() {
			vector<int> seq(_src.size());
			// 0_����˳��
			iota(seq.begin(), seq.end(), 0);
			_sort_rules.reserve(5);
			for (int i = 0; i < 5; ++i) { _sort_rules.push_back({ seq, numeric_limits<double>::max() }); }
			// 1_����ݼ�
			sort(_sort_rules[1].sequence.begin(), _sort_rules[1].sequence.end(), [this](int lhs, int rhs) {
				return _ins.get_blocks().at(lhs).area > _ins.get_blocks().at(rhs).area; });
			// 2_�߶ȵݼ�
			sort(_sort_rules[2].sequence.begin(), _sort_rules[2].sequence.end(), [this](int lhs, int rhs) {
				return _src.at(lhs).height > _src.at(rhs).height; });
			// 3_��ȵݼ�
			sort(_sort_rules[3].sequence.begin(), _sort_rules[3].sequence.end(), [this](int lhs, int rhs) {
				return _src.at(lhs).width > _src.at(rhs).width; });
			// 4_�������
			shuffle(_sort_rules[4].sequence.begin(), _sort_rules[4].sequence.end(), _gen);

			// `_rects`Ĭ������˳��
			_rects.assign(_sort_rules[0].sequence.begin(), _sort_rules[0].sequence.end());

			// ��ɢ���ʷֲ���ʼ��
			vector<int> probs; probs.reserve(_sort_rules.size());
			for (int i = 1; i <= _sort_rules.size(); ++i) { probs.push_back(2 * i); }
			_discrete_dist = discrete_distribution<>(probs.begin(), probs.end());
			// ���ȷֲ���ʼ��
			_uniform_dist = uniform_int_distribution<>(0, _src.size() - 1);
		}

		/// ������1�������������˳��
		void swap_sort_rule(SortRule &rule) {
			int a = _uniform_dist(_gen);
			int b = _uniform_dist(_gen);
			while (a == b) { b = _uniform_dist(_gen); }
			swap(rule.sequence[a], rule.sequence[b]);
		}

		/// ������2������������ƶ�
		void rotate_sort_rule(SortRule &rule) {
			int a = _uniform_dist(_gen);
			rotate(rule.sequence.begin(), rule.sequence.begin() + a, rule.sequence.end());
		}

		/// ���ڷ��������ѡ��ѡ���Σ���С������ģ
		list<int> get_candidate_rects(int skyline_index, Config::LevelGroupSearch method) {
			if (method == Config::LevelGroupSearch::NoGroup) { return _rects; }

			int gid = 0;
			for (; gid < _group_boundaries.size(); ++gid) {
				if (_skyline[skyline_index].x >= _group_boundaries[gid].x
					&& _skyline[skyline_index].y >= _group_boundaries[gid].y
					&& _skyline[skyline_index].x < _group_boundaries[gid].x + _group_boundaries[gid].width
					&& _skyline[skyline_index].y < _group_boundaries[gid].y + _group_boundaries[gid].height) {
					break;
				}
			}
			if (gid == _group_boundaries.size()) { // �����ϱ߽磬�Ӻ���ǰֻ���x��Χ����
				--gid;
				for (; gid >= 0; --gid) {
					if (_skyline[skyline_index].x >= _group_boundaries[gid].x
						&& _skyline[skyline_index].y >= _group_boundaries[gid].y
						&& _skyline[skyline_index].x < _group_boundaries[gid].x + _group_boundaries[gid].width) {
						break;
					}
				}
			}
			list<int> candidate_rects;
			switch (method) {
			case Config::LevelGroupSearch::NeighborNone:
				for (int r : _rects) { // ����˳�����_rects����ʹsort_rule��Ч
					if (_src.at(r).gid == gid)
						candidate_rects.push_back(r);
				}
				break;
			case Config::LevelGroupSearch::NeighborAll:
				for (int r : _rects) {
					if (_src.at(r).gid == gid)
						candidate_rects.push_back(r);
					else if (_group_neighbors[_src.at(r).gid][gid])
						candidate_rects.push_back(r);
				}
				break;
			case Config::LevelGroupSearch::NeighborPartial:
				for (int r : _rects) {
					if (_src.at(r).gid == gid)
						candidate_rects.push_back(r);
					else if (_group_neighbors[_src.at(r).gid][gid] && _src.at(r).gid < gid)
						candidate_rects.push_back(r);
				}
				break;
			default:
				assert(false);
				break;
			}
			// ���û�з��ϲ��Եľ��Σ��򷵻����о���
			if (candidate_rects.empty()) { return _rects; }
			return candidate_rects;
		}

		/// �������ºʹ�ֲ���
		int find_rect_for_skyline_bottom_left(int skyline_index, const list<int> &rects, vector<Rect> &dst) {
			int best_rect = -1, best_score = -1;
			for (int r : rects) {
				int width = _src.at(r).width, height = _src.at(r).height;
				int x, score;
				for (int rotate = 0; rotate <= 1; ++rotate) {
					if (rotate) { swap(width, height); }
					if (score_rect_for_skyline_bottom_left(skyline_index, width, height, x, score)) {
						if (best_score < score) {
							best_score = score;
							best_rect = r;
							dst[r].x = x;
							dst[r].y = _skyline[skyline_index].y;
							dst[r].width = width;
							dst[r].height = height;
						}
					}
				}
			}
			// (d)(f)(h)���˻����
			if (best_score == 4 || best_score == 2 || best_score == 0) {
				int min_unpacked_width = numeric_limits<int>::max();
				for (int r : rects) {
					if (r != best_rect) { min_unpacked_width = min(min_unpacked_width, _src.at(r).width); }
				}
				// δ���õ���С��ȷŲ��£������˷�
				if (min_unpacked_width > _skyline[skyline_index].width - dst[best_rect].width) {
					SkylineSpace space = skyline_nodo_to_space(_skyline, skyline_index);
					int min_space_height = min(space.hl, space.hr);
					int new_best_rect = -1;
					int new_best_width = 0, new_best_height;
					for (int r : rects) {
						for (int rotate = 0; rotate <= 1; ++rotate) {
							int width = _src.at(r).width, height = _src.at(r).height;
							if (rotate) { swap(width, height); }
							if (height >= min_space_height // �߲�С��min_space_height
								&& width <= space.width // ���ܷ���
								&& width > new_best_width) { // ���
								new_best_rect = r;
								new_best_width = width;
								new_best_height = height;
							}
						}
					}
					if (new_best_rect != -1) {
						best_rect = new_best_rect;
						dst[best_rect].x = space.hl >= space.hr ? _skyline[skyline_index].x : // ����
							_skyline[skyline_index].x + _skyline[skyline_index].width - new_best_width; // ����
						dst[best_rect].y = _skyline[skyline_index].y;
						dst[best_rect].width = new_best_width;
						dst[best_rect].height = new_best_height;
					}
				}
			}

			return best_rect;
		}

		/// ��ֲ���
		bool score_rect_for_skyline_bottom_left(int skyline_index, int width, int height, int &x, int &score) {
			if (width > _skyline[skyline_index].width) { return false; }

			SkylineSpace space = skyline_nodo_to_space(_skyline, skyline_index);
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

				if (score == 2) { x = _skyline[skyline_index].x + _skyline[skyline_index].width - width; }
				else { x = _skyline[skyline_index].x; }
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

				if (score == 4 || score == 0) { x = _skyline[skyline_index].x + _skyline[skyline_index].width - width; }
				else { x = _skyline[skyline_index].x; }
			}
			if (x + width > _bin_width) { return false; }

			return true;
		}

	private:
		Skyline _skyline;

		// ��������б���������ֲ�����  
		vector<SortRule> _sort_rules;
		list<int> _rects; // SortRule��sequence���൱��ָ�룬ʹ��list����ɾ�����������Ϊ��
		discrete_distribution<> _discrete_dist;   // ��ɢ���ʷֲ���������ѡ����(����ѡsequence����_rects)
		uniform_int_distribution<> _uniform_dist; // ���ȷֲ������ڽ�������˳��

		// ������Ϣ������`get_candidate_rects()`��ѡ��ѡ����
		vector<Boundary> _group_boundaries;    // `_group_boundaries[i]`   ����_i�ı߽�
		vector<vector<bool>> _group_neighbors; // `_group_neighbors[i][j]` ����_i��_j�Ƿ�Ϊ�ھ�
	};

}
