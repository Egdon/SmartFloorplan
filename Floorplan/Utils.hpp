//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace utils {

	using namespace std;

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

	// �����ļ��е�n�� 
	static void skip(FILE *file, int line_num) {
		char linebuf[1000];
		for (int i = 0; i < line_num; ++i) {
			fgets(linebuf, sizeof(linebuf), file); // skip
		}
	}

	// '01'����������� [todo] bugֻ�ܵ���һ��
	bool next_combination(const vector<int> &a, int n, int k, vector<int> &comb, vector<int> &ncomb) {
		static vector<bool> index(n, false);
		static bool first_comb = true;  // ��һ����ϣ�ѡ��ǰk��λ��
		static auto has_done = [=]() {  // ��ֹ������  ���k��λ��ȫ���1
			for (int i = n - 1; i >= n - k; i--)
				if (!index[i])
					return false;
			return true;
		};

		if (first_comb) {
			for (int i = 0; i < k; ++i) { index[i] = true; }
			comb.clear(); comb.reserve(k);
			ncomb.clear(); ncomb.reserve(n - k);
			for (int i = 0; i < n; ++i) {
				if (index[i])
					comb.push_back(a[i]);
				else
					ncomb.push_back(a[i]);
			}
			first_comb = false;
			return true;
		}

		if (!has_done()) {
			for (int i = 0; i < n - 1; ++i) {
				// �ҵ���һ����10����Ͻ�����"01"���
				if (index[i] && !index[i + 1]) {
					index[i] = false;
					index[i + 1] = true;
					// ��"01"�����ߵ�1�Ƶ������
					int count = 0;
					for (int j = 0; j < i; ++j) {
						if (index[j]) {
							index[j] = false;
							index[count++] = true;
						}
					}
					// ��¼��ǰ���
					comb.clear();
					ncomb.clear();
					for (int l = 0; l < n; ++l) {
						if (index[l])
							comb.push_back(a[l]);
						else
							ncomb.push_back(a[l]);
					}
						
					return true;
				}
			}
		}

		return false;
	}
}
