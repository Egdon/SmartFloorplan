//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#include "FloorplanPacker.hpp"

namespace fbp {

	using namespace std;

	class BeamSearcher : public FloorplanPacker {

		/// �����ò�ڵ㶨��
		struct BeamNode {
			vector<Rect> dst; // �ѷ��þ��Σ�partial/complete solution
			list<int> rects;  // δ���þ���
			vector<bool> is_packed;
			Netwire netwire;
			Skyline skyline;
			int bl_index; // bottom_left_skyline_index
		};

		/// �²�ڵ㶨��
		struct BranchNode {
			const BeamNode *parent;
			int chosen_rect_index; // ѡ�еľ���
			int chosen_rect_width; // ��ת֮��Ŀ��
			int chosen_rect_height; // ͬ��
			int chosen_rect_xcoord; // ѡ�о��ε�x���꣨����/�ҷ��ã�
			int area_score; // �����֣�max
			int wire_score; // �߳���֣�max
			double local_eval; // �ֲ���������ֲ��ԣ�max
			double global_eval; // ȫ��������Ŀ�꺯����min
			double look_ahead_eval; // ��ǰ��������Ŀ�꺯����min
		};

	public:
		BeamSearcher() = delete;

		BeamSearcher(const Instance &ins, const vector<Rect> &src, int bin_width, const vector<vector<int>> &graph, default_random_engine &gen) :
			FloorplanPacker(ins, src, bin_width, graph, gen) {}

		void run(int beam_width, double alpha, double beta, Config::LevelWireLength level_wl, Config::LevelObjDist level_dist,
			Config::LevelGroupSearch level_gs = Config::LevelGroupSearch::NoGroup) {

			reset_beam_tree();
			int filter_width = beam_width * 2;
			while (!_beam_tree.front().rects.empty()) {
				vector<BranchNode> filter_children; filter_children.reserve(_beam_tree.size() * _src.size() * 2);
				for (auto &parent : _beam_tree) {
					check_parent(parent);
					vector<BranchNode> children = branch(parent, level_dist);
					// 1.�ֲ���������ȫ���ӽڵ���ѡ��`filter_width`����ÿ�����ڵ㹱��`filter_width/_beam_tree.size()`��
					if (children.size() > filter_width / _beam_tree.size()) {
						local_evaluation(children, alpha, beta);
						nth_element(children.begin(), children.begin() + filter_width / _beam_tree.size() - 1, children.end(),
							[](auto &lhs, auto &rhs) { return lhs.local_eval > rhs.local_eval; });
						auto nth_iter = children.begin() + filter_width / _beam_tree.size() - 1;
						auto left_iter = nth_iter, right_iter = nth_iter;
						while (left_iter->local_eval == nth_iter->local_eval && left_iter != children.begin()) { left_iter = prev(left_iter); }
						while (right_iter->local_eval == nth_iter->local_eval && right_iter != children.end()) { right_iter = next(right_iter); }
						if (left_iter != children.begin()) { ++left_iter; }
						shuffle(left_iter, right_iter, _gen); // ͬ���������ȡ
						//shuffle(children.begin(), right_iter, _gen); // ȫ���������ȡ
						filter_children.insert(filter_children.end(), children.begin(), next(nth_iter));
					}
					// ����`filter_width/_beam_tree.size()`����ȫѡ��
					else { filter_children.insert(filter_children.end(), children.begin(), children.end()); }
				}

				for_each(filter_children.begin(), filter_children.end(), [=](auto &child) {
					global_evaluation(child, alpha, beta, level_wl, level_dist);
					look_ahead_evaluation(child, alpha, beta, level_wl, level_dist);
				});

				vector<BranchNode> beam_children; beam_children.reserve(filter_children.size());
				if (beam_width == 1) {
					beam_children.push_back(*min_element(filter_children.begin(), filter_children.end(),
						[](auto &lhs, auto &rhs) { return lhs.global_eval < rhs.global_eval; }));
				}
				else if (filter_children.size() > beam_width) {
					// 2.ȫ����������`filter_children`��ѡ��`beam_width/2`��
					nth_element(filter_children.begin(), filter_children.begin() + beam_width / 2 - 1, filter_children.end(),
						[](auto &lhs, auto &rhs) { return lhs.global_eval < rhs.global_eval; });
					beam_children.insert(beam_children.end(), filter_children.begin(), filter_children.begin() + beam_width / 2);
					// 3.����ǰ����������ʣ��`filter_children`��ѡ��`beam_width/2`��
					nth_element(filter_children.begin() + beam_width / 2, filter_children.begin() + beam_width - 1, filter_children.end(),
						[](auto &lhs, auto &rhs) { return lhs.look_ahead_eval < rhs.look_ahead_eval; });
					beam_children.insert(beam_children.end(), filter_children.begin() + beam_width / 2, filter_children.begin() + beam_width);
				}
				else { beam_children.insert(beam_children.end(), filter_children.begin(), filter_children.end()); }

				// 4.ִ��ѡ�ж���������������һ��
				vector<BeamNode> new_beam_tree; new_beam_tree.reserve(beam_children.size());
				for (auto &child : beam_children) {
					BeamNode parent_copy = *child.parent;
					insert_chosen_rect_for_parent(parent_copy, child.chosen_rect_index,
						child.chosen_rect_width, child.chosen_rect_height, child.chosen_rect_xcoord);
					new_beam_tree.push_back(move(parent_copy));
				}
				_beam_tree = new_beam_tree;
			}
		}

	private:
		/// ÿ�ε�������_beam_tree
		void reset_beam_tree() {
			_beam_tree.clear();
			BeamNode root;
			root.dst = _src;
			root.rects.resize(_src.size());
			iota(root.rects.begin(), root.rects.end(), 0);
			root.is_packed.resize(_src.size(), false);
			root.netwire.resize(_ins.get_net_num());
			for_each(root.netwire.begin(), root.netwire.end(), [](auto &netwire_node) {
				netwire_node.max_x = netwire_node.max_y = 0;
				netwire_node.min_x = netwire_node.min_y = numeric_limits<double>::max();
				netwire_node.hpwl = 0.0;
			});
			root.skyline.clear();
			root.skyline.push_back({ 0,0,_bin_width });
			root.bl_index = 0;
			_beam_tree.push_back(move(root));
		}

		/// ������ & ����`bl_index`
		void check_parent(BeamNode &parent) {
			int bottom_skyline_index;
			int min_rect_width = _src.at(*min_element(parent.rects.begin(), parent.rects.end(),
				[this](int lhs, int rhs) { return _src.at(lhs).width < _src.at(rhs).width; })).width;
			while (1) {
				auto bottom_skyline_iter = min_element(parent.skyline.begin(), parent.skyline.end(),
					[](auto &lhs, auto &rhs) { return lhs.y < rhs.y; });
				bottom_skyline_index = distance(parent.skyline.begin(), bottom_skyline_iter);

				if (parent.skyline[bottom_skyline_index].width < min_rect_width) { // ��С��Ⱦ��ηŲ���ȥ����Ҫ���
					if (bottom_skyline_index == 0) {
						parent.skyline[bottom_skyline_index].y = parent.skyline[bottom_skyline_index + 1].y;
					}
					else if (bottom_skyline_index == parent.skyline.size() - 1) {
						parent.skyline[bottom_skyline_index].y = parent.skyline[bottom_skyline_index - 1].y;
					}
					else {
						parent.skyline[bottom_skyline_index].y = min(parent.skyline[bottom_skyline_index - 1].y,
							parent.skyline[bottom_skyline_index + 1].y);
					}
					merge_skylines(parent.skyline);
					continue;
				}
				break;
			}
			parent.bl_index = bottom_skyline_index;
		}

		/// ��֧����
		vector<BranchNode> branch(const BeamNode &parent, Config::LevelObjDist level_dist) {
			vector<BranchNode> children; children.reserve(parent.rects.size() * 2);
			for (int r : parent.rects) {
				for (int rotate = 0; rotate <= 1; ++rotate) {
					BranchNode child;
					child.parent = &parent;
					child.chosen_rect_index = r;
					child.chosen_rect_width = rotate ? _src.at(r).height : _src.at(r).width;
					child.chosen_rect_height = rotate ? _src.at(r).width : _src.at(r).height;
					if (score_area_and_set_xcoord(parent, child.chosen_rect_width, child.chosen_rect_height,
						child.chosen_rect_xcoord, child.area_score)) {
						child.wire_score = score_wire(child, level_dist);
						children.push_back(move(child));
					}
				}
			}
			return children;
		}

		/// �ֲ���������ֲ���
		void local_evaluation(vector<BranchNode> &children, double alpha, double beta) {
			vector<int> area_rank(children.size());
			vector<int> wire_rank(children.size());
			iota(area_rank.begin(), area_rank.end(), 0);
			iota(wire_rank.begin(), wire_rank.end(), 0);
			sort(area_rank.begin(), area_rank.end(), [&](int lhs, int rhs) { return children[lhs].area_score < children[rhs].area_score; });
			sort(wire_rank.begin(), wire_rank.end(), [&](int lhs, int rhs) { return children[lhs].wire_score < children[rhs].wire_score; });
			for (int i = 0; i < children.size(); ++i) {
				children[i].local_eval =
					alpha * distance(area_rank.begin(), find(area_rank.begin(), area_rank.end(), i)) +
					beta * distance(wire_rank.begin(), find(wire_rank.begin(), wire_rank.end(), i));
			}
		}

		/// ȫ��������̰���ߵ��ף�Ŀ�꺯��
		void global_evaluation(BranchNode &child, double alpha, double beta,
			Config::LevelWireLength level_wl, Config::LevelObjDist level_dist) {
			BeamNode parent_copy = *child.parent;
			insert_chosen_rect_for_parent(parent_copy, child.chosen_rect_index,
				child.chosen_rect_width, child.chosen_rect_height, child.chosen_rect_xcoord);
			int target_area = greedy_construction(parent_copy, false) * _bin_width;
			double dist;
			double target_wirelength = cal_wirelength(parent_copy.dst, parent_copy.is_packed, dist, level_wl, level_dist);
			child.global_eval = cal_objective(target_area, dist, alpha, beta);
			update_objective(child.global_eval, target_area, target_wirelength, parent_copy.dst);
		}

		/// ��ǰ��������Ŀ�꺯��
		void look_ahead_evaluation(BranchNode &child, double alpha, double beta,
			Config::LevelWireLength level_wl, Config::LevelObjDist level_dist) {
			BeamNode parent_copy = *child.parent;
			insert_chosen_rect_for_parent(parent_copy, child.chosen_rect_index,
				child.chosen_rect_width, child.chosen_rect_height, child.chosen_rect_xcoord);
			int target_area = greedy_construction(parent_copy, true) * _bin_width;
			double dist;
			double target_wirelength = cal_wirelength(parent_copy.dst, parent_copy.is_packed, dist, level_wl, level_dist);
			child.look_ahead_eval = cal_objective(target_area, dist, alpha, beta);
			if (parent_copy.rects.empty()) { update_objective(child.look_ahead_eval, target_area, target_wirelength, parent_copy.dst); }
		}

		/// �����ֲ��� & ����`rect_xcoord`
		bool score_area_and_set_xcoord(const BeamNode &parent, int width, int height, int &x, int &score) {
			if (width > parent.skyline[parent.bl_index].width) { return false; }

			SkylineSpace space = skyline_nodo_to_space(parent.skyline, parent.bl_index);
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

				if (score == 2) { x = parent.skyline[parent.bl_index].x + parent.skyline[parent.bl_index].width - width; }
				else { x = parent.skyline[parent.bl_index].x; }
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

				if (score == 4 || score == 0) { x = parent.skyline[parent.bl_index].x + parent.skyline[parent.bl_index].width - width; }
				else { x = parent.skyline[parent.bl_index].x; }
			}
			if (x + width > _bin_width) { return false; }

			return true;
		}

		/// �߳���ֲ���
		double score_wire(BranchNode &node, Config::LevelObjDist level_dist) {
			double avg_length;
			double pin_x = node.chosen_rect_xcoord + node.chosen_rect_width * 0.5;
			double pin_y = node.parent->skyline[node.parent->bl_index].y + node.chosen_rect_height * 0.5;
			switch (level_dist) {
			case Config::LevelObjDist::SqrEuclideanDist: {
				int wire_num = 0;
				double wire_length = 0;
				for (int i = 0; i < _graph.size(); ++i) {
					if (_graph[i][node.chosen_rect_index] && node.parent->is_packed[i]) {
						wire_num += 1; // ������ͬʱ���ڶ�����������ظ����߽���һ��
						wire_length += qapc::cal_distance(Config::LevelDist::EuclideanDist, pin_x, pin_y,
							node.parent->dst[i].x + node.parent->dst[i].width * 0.5,
							node.parent->dst[i].y + node.parent->dst[i].height * 0.5);
					}
				}
				avg_length = wire_length / wire_num;
				break;
			}
			case Config::LevelObjDist::SqrManhattanDist: {
				int wire_num = 0;
				double wire_length = 0;
				for (int i = 0; i < _graph.size(); ++i) {
					if (_graph[i][node.chosen_rect_index] && node.parent->is_packed[i]) {
						wire_num += 1; // ������ͬʱ���ڶ�����������ظ����߽���һ��
						wire_length += qapc::cal_distance(Config::LevelDist::ManhattanDist, pin_x, pin_y,
							node.parent->dst[i].x + node.parent->dst[i].width * 0.5,
							node.parent->dst[i].y + node.parent->dst[i].height * 0.5);
					}
				}
				avg_length = wire_length / wire_num;
				break;
			}
			case Config::LevelObjDist::SqrHpwlDist: {
				int delta_net_num = 0;
				double delta_net_length = 0;
				for (int nid : _ins.get_blocks().at(node.chosen_rect_index).net_ids) {
					double max_x = max(node.parent->netwire[nid].max_x, pin_x);
					double min_x = min(node.parent->netwire[nid].min_x, pin_x);
					double max_y = max(node.parent->netwire[nid].max_y, pin_y);
					double min_y = min(node.parent->netwire[nid].min_y, pin_y);
					delta_net_length += (max_x - min_x + max_y - min_y) - node.parent->netwire[nid].hpwl;
					delta_net_num += delta_net_length ? 1 : 0; // û�б仯����������
				}
				avg_length = delta_net_length / delta_net_num;
				break;
			}
			default:
				assert(false);
				break;
			}

			return avg_length;
		}

		/// ִ��ѡ�еĶ���������parent
		int insert_chosen_rect_for_parent(BeamNode &parent, int rect_index, int rect_width, int rect_height, int rect_xcoord) {
			// ִ�з���
			parent.dst[rect_index].x = rect_xcoord;
			parent.dst[rect_index].y = parent.skyline[parent.bl_index].y;
			parent.dst[rect_index].width = rect_width;
			parent.dst[rect_index].height = rect_height;

			// ��δ�����б���ɾ��
			parent.rects.remove(rect_index);
			parent.is_packed[rect_index] = true;

			// ����skyline
			SkylineNode new_skyline_node{
				parent.dst[rect_index].x,
				parent.dst[rect_index].y + parent.dst[rect_index].height,
				parent.dst[rect_index].width
			};
			if (new_skyline_node.x == parent.skyline[parent.bl_index].x) { // ����
				parent.skyline.insert(parent.skyline.begin() + parent.bl_index, new_skyline_node);
				parent.skyline[parent.bl_index + 1].x += new_skyline_node.width;
				parent.skyline[parent.bl_index + 1].width -= new_skyline_node.width;
				merge_skylines(parent.skyline);
			}
			else { // ����
				parent.skyline.insert(parent.skyline.begin() + parent.bl_index + 1, new_skyline_node);
				parent.skyline[parent.bl_index].width -= new_skyline_node.width;
				merge_skylines(parent.skyline);
			}

			// ����netwire
			double pin_x = parent.dst[rect_index].x + parent.dst[rect_index].width * 0.5;
			double pin_y = parent.dst[rect_index].y + parent.dst[rect_index].height * 0.5;
			for (int nid : _ins.get_blocks().at(rect_index).net_ids) {
				NetwireNode &netwire_node = parent.netwire[nid];
				netwire_node.max_x = max(netwire_node.max_x, pin_x);
				netwire_node.min_x = min(netwire_node.min_x, pin_x);
				netwire_node.max_y = max(netwire_node.max_y, pin_y);
				netwire_node.min_y = min(netwire_node.min_y, pin_y);
				netwire_node.hpwl = max(0.0, netwire_node.max_x - netwire_node.min_x + netwire_node.max_y - netwire_node.min_y);
			}

			return new_skyline_node.y;
		}

		/// ̰�Ĺ���һ������/�ֲ��⣬���Ǵ��㹹����ǲ�ȫ�ֲ���
		int greedy_construction(BeamNode &parent, bool is_partial) {
			int max_skyline_height = max_element(parent.skyline.begin(), parent.skyline.end(),
				[](auto &lhs, auto &rhs) { return lhs.y < rhs.y; })->y;
			int stop_skyline_height = max_skyline_height;

			while (!parent.rects.empty()) {
				check_parent(parent);
				if (is_partial && parent.skyline[parent.bl_index].y >= stop_skyline_height) {
					break; // ���skyline����stop_skyline_height
				}
				int rect_index = -1, rect_width, rect_height, rect_xcoord;;
				find_rect_for_parent(parent, rect_index, rect_width, rect_height, rect_xcoord);
				assert(rect_index != -1);
				max_skyline_height = max(max_skyline_height,
					insert_chosen_rect_for_parent(parent, rect_index, rect_width, rect_height, rect_xcoord));
			}

			return max_skyline_height;
		}

		/// Ϊ��ǰ��̰��ѡһ����
		void find_rect_for_parent(const BeamNode &parent, int &rect_index, int &rect_width, int &rect_height, int &rect_xcoord) {
			int best_score = -1;
			for (int r : parent.rects) {
				int width = _src.at(r).width, height = _src.at(r).height;
				int xcoord, score;
				for (int rotate = 0; rotate <= 1; ++rotate) {
					if (rotate) { swap(width, height); }
					if (score_area_and_set_xcoord(parent, width, height, xcoord, score)) {
						if (best_score < score) {
							best_score = score;
							rect_index = r;
							rect_width = width;
							rect_height = height;
							rect_xcoord = xcoord;
						}
					}
				}
			}
			// (d)(f)(h)���˻����
			if (best_score == 4 || best_score == 2 || best_score == 0) {
				int min_unpacked_width = numeric_limits<int>::max();
				for (int r : parent.rects) {
					if (r != rect_index) { min_unpacked_width = min(min_unpacked_width, _src.at(r).width); }
				}
				// δ���õ���С��ȷŲ��£������˷�
				if (min_unpacked_width > parent.skyline[parent.bl_index].width - rect_width) {
					SkylineSpace space = skyline_nodo_to_space(parent.skyline, parent.bl_index);
					int min_space_height = min(space.hl, space.hr);
					int new_best_rect = -1;
					int new_best_width = 0, new_best_height;
					for (int r : parent.rects) {
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
						rect_index = new_best_rect;
						rect_width = new_best_width;
						rect_height = new_best_height;
						rect_xcoord = space.hl >= space.hr ?
							parent.skyline[parent.bl_index].x : // ���󣨱��뿿�ߵ�һ��ţ�
							parent.skyline[parent.bl_index].x + parent.skyline[parent.bl_index].width - rect_width; // ����
					}
				}
			}
		}

	private:
		vector<BeamNode> _beam_tree;
	};

}
