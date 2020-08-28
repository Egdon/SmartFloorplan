//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>

const int INF = 0x3f3f3f3f;

namespace utils {

	using namespace std;

	// �����ļ��е�n�� 
	static void skip(FILE *file, int line_num) {
		char linebuf[1000];
		for (int i = 0; i < line_num; ++i) {
			fgets(linebuf, sizeof(linebuf), file); // skip
		}
	}

	class Date {
	public:
		// ���ر�ʾ���ڸ�ʽ���ַ�������-��-��
		static const string to_short_str() {
			ostringstream os;
			time_t now = time(0);
			os << put_time(localtime(&now), "%y-%m-%e");
			return os.str();
		}
		// ���ر�ʾ���ڸ�ʽ���ַ�������������
		static const string to_detail_str() {
			ostringstream os;
			time_t now = time(0);
			//os << put_time(localtime(&now), "%y-%m-%e-%H_%M_%S");
			os << put_time(localtime(&now), "%Y%m%d%H%M%S");
			return os.str();
		}
	};

	class Combination {
	public:
		Combination(const vector<int> &a, int k) : _a(a), _n(a.size()), _k(k), _index(a.size(), false), _first_comb(true) {}

		bool next_combination(vector<int> &comb, vector<int> &ncomb) {
			if (_first_comb) { // ��һ����ϣ�ѡ��ǰk��λ��
				_first_comb = false;
				for (int i = 0; i < _k; ++i) { _index[i] = true; }
				record_combination(comb, ncomb);
				return true;
			}

			if (!has_done()) {
				for (int i = 0; i < _n - 1; ++i) {
					// �ҵ���һ����10����Ͻ�����"01"���
					if (_index[i] && !_index[i + 1]) {
						_index[i] = false;
						_index[i + 1] = true;
						// ��"01"�����ߵ�1�Ƶ������
						int count = 0;
						for (int j = 0; j < i; ++j) {
							if (_index[j]) {
								_index[j] = false;
								_index[count++] = true;
							}
						}
						record_combination(comb, ncomb);
						return true;
					}
				}
			}

			return false;
		}

	private:
		bool has_done() const { // ��ֹ���������k��λ��ȫ���1
			for (int i = _n - 1; i >= _n - _k; --i)
				if (!_index[i])
					return false;
			return true;
		}

		void record_combination(vector<int> &comb, vector<int> &ncomb) const {
			comb.clear(); comb.reserve(_k);
			ncomb.clear(); ncomb.reserve(_n - _k);
			for (int i = 0; i < _n; ++i) {
				if (_index[i])
					comb.push_back(_a[i]);
				else
					ncomb.push_back(_a[i]);
			}
		}

	private:
		const vector<int> &_a;
		const int _n;
		const int _k;
		vector<bool> _index;
		bool _first_comb;
	};
}
