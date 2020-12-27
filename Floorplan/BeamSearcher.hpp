//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#include "FloorplanPacker.hpp"

namespace fbp {

	using namespace std;

	class BeamSearcher : public FloorplanPacker {

		/// Space����
		struct SkylineSpace {
			int x;
			int y;
			int width;
			int hl;
			int hr;
		};

		static SkylineSpace skyline_nodo_to_space(const Skyline &skyline, int skyline_index) {
			int hl, hr;
			if (skyline.size() == 1) {
				hl = hr = INF - skyline[skyline_index].y;
			}
			else if (skyline_index == 0) {
				hl = INF - skyline[skyline_index].y;
				hr = skyline[skyline_index + 1].y - skyline[skyline_index].y;
			}
			else if (skyline_index == skyline.size() - 1) {
				hl = skyline[skyline_index - 1].y - skyline[skyline_index].y;
				hr = INF - skyline[skyline_index].y;
			}
			else {
				hl = skyline[skyline_index - 1].y - skyline[skyline_index].y;
				hr = skyline[skyline_index + 1].y - skyline[skyline_index].y;
			}
			return { skyline[skyline_index].x, skyline[skyline_index].y, skyline[skyline_index].width, hl, hr };
		}

		/// �����ò�ڵ㶨��
		struct BeamNode {
			vector<Rect> dst; // �ѷ��þ��Σ�partial/complete solution
			list<int> rects;  // δ���þ���
			vector<bool> is_packed;
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
			int area_score; // ������
			int wire_score; // �߳����
			double local_eval; // �ֲ���������ֲ���
			double global_eval; // ȫ��������Ŀ�꺯��
			double look_ahead_eval; // ��ǰ��������Ŀ�꺯��
		};

	public:
		BeamSearcher() = delete;

		BeamSearcher(const Instance &ins, const vector<Rect> &src, int bin_width, default_random_engine &gen,
			const vector<vector<int>> &graph) :
			FloorplanPacker(ins, src, bin_width, gen), _graph(graph) {
			init_beam_tree();
		}

		void run(int beam_width, double alpha, double beta, Config::LevelWireLength level_wl,
			Config::LevelGroupSearch level_gs = Config::LevelGroupSearch::NoGroup) {
			int filter_width = beam_width * 2;
			while (!_beam_tree.front().rects.empty()) {
				vector<BranchNode> filter_children; filter_children.reserve(filter_width);
				vector<BranchNode> beam_children; beam_children.reserve(beam_width);

				for (auto &parent : _beam_tree) {
					check_parent(parent);
					vector<BranchNode> children = branch(parent);
					local_evaluation(children);
					// 1.�ֲ���������ȫ���ӽڵ���ѡ��`filter_width`����ÿ�����ڵ㹱��`filter_width/_beam_tree.size()`��
					if (children.size() > filter_width / _beam_tree.size()) {
						nth_element(children.begin(), children.begin() + filter_width / _beam_tree.size() - 1, children.end(),
							[](auto &lhs, auto &rhs) { return lhs.local_eval < rhs.local_eval; });
						filter_children.insert(filter_children.end(), children.begin(), children.begin() + filter_width / _beam_tree.size());
					}
					else { // ����`filter_width/_beam_tree.size()`����ȫѡ��
						filter_children.insert(filter_children.end(), children.begin(), children.end());
					}
				}

				for_each(filter_children.begin(), filter_children.end(), [=](auto &child) {
					global_evaluation(child, alpha, beta, level_wl);
					look_ahead_evaluation(child, alpha, beta, level_wl);
				});

				if (beam_width == 1) {
					beam_children.push_back(*min_element(filter_children.begin(), filter_children.end(),
						[](auto &lhs, auto &rhs) { return lhs.global_eval < rhs.global_eval; }));
				}
				else {
					// 2.ȫ����������`filter_children`��ѡ��`beam_width/2`��
					nth_element(filter_children.begin(), filter_children.begin() + beam_width / 2 - 1, filter_children.end(),
						[](auto &lhs, auto &rhs) { return lhs.global_eval < rhs.global_eval; });
					beam_children.insert(beam_children.end(), filter_children.begin(), filter_children.begin() + beam_width / 2);
					// 3.����ǰ����������ʣ��`filter_children`��ѡ��`beam_width/2`��
					nth_element(filter_children.begin() + beam_width / 2, filter_children.begin() + beam_width - 1, filter_children.end(),
						[](auto &lhs, auto &rhs) { return lhs.look_ahead_eval < rhs.look_ahead_eval; });
					beam_children.insert(beam_children.end(), filter_children.begin() + beam_width / 2, filter_children.begin() + beam_width);
				}

				// 4.ִ��ѡ�ж���������������һ��
				vector<BeamNode> new_beam_tree; new_beam_tree.reserve(beam_width);
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
		void init_beam_tree() {
			BeamNode root;
			root.dst = _src;
			root.rects.resize(_src.size());
			iota(root.rects.begin(), root.rects.end(), 0);
			root.is_packed.resize(_src.size(), false);
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
		vector<BranchNode> branch(const BeamNode &parent) {
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
						child.wire_score = score_wire(child);
						children.push_back(move(child));
					}
				}
			}
			return children;
		}

		/// �ֲ���������ֲ���
		void local_evaluation(vector<BranchNode> &children) {
			vector<int> area_rank(children.size());
			vector<int> wire_rank(children.size());
			iota(area_rank.begin(), area_rank.end(), 0);
			iota(wire_rank.begin(), wire_rank.end(), 0);
			sort(area_rank.begin(), area_rank.end(), [&](int lhs, int rhs) { return children[lhs].area_score > children[rhs].area_score; });
			sort(wire_rank.begin(), wire_rank.end(), [&](int lhs, int rhs) { return children[lhs].wire_score > children[rhs].wire_score; });
			for (int i = 0; i < children.size(); ++i) {
				children[i].local_eval = distance(area_rank.begin(), find(area_rank.begin(), area_rank.end(), i))
					+ distance(wire_rank.begin(), find(wire_rank.begin(), wire_rank.end(), i));
			}
		}

		/// ȫ��������̰���ߵ��ף�Ŀ�꺯��
		void global_evaluation(BranchNode &child, double alpha, double beta, Config::LevelWireLength level_wl) {
			BeamNode parent_copy = *child.parent;
			insert_chosen_rect_for_parent(parent_copy, child.chosen_rect_index,
				child.chosen_rect_width, child.chosen_rect_height, child.chosen_rect_xcoord);
			int target_area = greedy_construction(parent_copy, false) * _bin_width;
			double dist;
			double target_wirelength = cal_wirelength(parent_copy, dist, level_wl);
			child.global_eval = cal_objective(target_area, dist, alpha, beta);
			update_objective(child.global_eval, target_area, target_wirelength, parent_copy);
		}

		/// ��ǰ��������Ŀ�꺯��
		void look_ahead_evaluation(BranchNode &child, double alpha, double beta, Config::LevelWireLength level_wl) {
			BeamNode parent_copy = *child.parent;
			insert_chosen_rect_for_parent(parent_copy, child.chosen_rect_index,
				child.chosen_rect_width, child.chosen_rect_height, child.chosen_rect_xcoord);
			int target_area = greedy_construction(parent_copy, true) * _bin_width;
			double dist;
			double target_wirelength = cal_wirelength(parent_copy, dist, level_wl);
			child.look_ahead_eval = cal_objective(target_area, dist, alpha, beta);
			update_objective(child.look_ahead_eval, target_area, target_wirelength, parent_copy);
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

		/// �߳���ֲ��ԣ�[todo] ʹ��������Ŀ or ���߳���
		int score_wire(BranchNode &node) {
			int wire_num = 0, wire_length = 0;
			for (int i = 0; i < _src.size(); ++i) {
				if (node.chosen_rect_index != i && node.parent->is_packed[i]) {
					wire_num += _graph[node.chosen_rect_index][i];
					wire_length += QAPCluster::cal_distance(Config::LevelDist::EuclideanDist,
						node.chosen_rect_xcoord + node.chosen_rect_width / 2,
						node.parent->skyline[node.parent->bl_index].y + node.chosen_rect_height / 2,
						node.parent->dst[i].x + node.parent->dst[i].width / 2,
						node.parent->dst[i].y + node.parent->dst[i].height / 2
					) * _graph[node.chosen_rect_index][i];
				}
			}
			int avg_length = wire_length / wire_num;
			return wire_num;
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

		/// �����߳���Ĭ������������
		double cal_wirelength(const BeamNode &parent, double &dist, Config::LevelWireLength method) {
			double total_wirelength = 0;
			dist = 0;
			for (auto &net : _ins.get_netlist()) {
				double max_x = 0, min_x = numeric_limits<double>::max();
				double max_y = 0, min_y = numeric_limits<double>::max();
				for (int b : net.block_list) {
					if (parent.is_packed.at(b)) { // ֻ���㵱ǰ�ѷ��õĿ�
						double pin_x = parent.dst.at(b).x + parent.dst.at(b).width * 0.5;
						double pin_y = parent.dst.at(b).y + parent.dst.at(b).height * 0.5;
						max_x = max(max_x, pin_x);
						min_x = min(min_x, pin_x);
						max_y = max(max_y, pin_y);
						min_y = min(min_y, pin_y);
					}
				}
				if (method == Config::LevelWireLength::BlockAndTerminal) {
					for (int t : net.terminal_list) {
						double pad_x = _ins.get_terminals().at(t).x_coordinate;
						double pad_y = _ins.get_terminals().at(t).y_coordinate;
						max_x = max(max_x, pad_x);
						min_x = min(min_x, pad_x);
						max_y = max(max_y, pad_y);
						min_y = min(min_y, pad_y);
					}
				}
				double hpwl = max_x - min_x + max_y - min_y;
				total_wirelength += hpwl;
				dist += hpwl * hpwl;
			}

			return total_wirelength;
		}

		/// Ŀ�꺯����EDAthon-2020-P4
		double cal_objective(int area, double dist, double alpha, double beta) {
			return (alpha * area + beta * dist) / _ins.get_total_area();
		}

		void update_objective(double objective, int area, double wire, const BeamNode &parent) {
			if (parent.rects.empty() && objective < _objective) {
				_objective = objective;
				_obj_area = area;
				_obj_fillratio = 1.0 * _ins.get_total_area() / _obj_area;
				_obj_wirelength = wire;
				_dst = parent.dst;
			}
		}

		/// �ϲ�ͬһlevel��skyline�ڵ�.
		static void merge_skylines(Skyline &skyline) {
			skyline.erase(
				remove_if(skyline.begin(), skyline.end(), [](auto &rhs) { return rhs.width <= 0; }),
				skyline.end()
			);
			for (int i = 0; i < skyline.size() - 1; ++i) {
				if (skyline[i].y == skyline[i + 1].y) {
					skyline[i].width += skyline[i + 1].width;
					skyline.erase(skyline.begin() + i + 1);
					--i;
				}
			}
		}

	private:
		const vector<vector<int>> &_graph; // �߳���֣�������֮�����ӵĽ��̶ܳ�
		vector<BeamNode> _beam_tree;
	};

}
