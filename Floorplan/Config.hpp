//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#ifdef NDEBUG
#define debug_run(x)
#else
#define debug_run(x) x
#endif // !NDEBUG

#include <iostream>
#include <vector>
#include <utility>
#include <string>
#include <random>
#include <unordered_map>

static constexpr int INF = 0x3f3f3f3f;

static std::vector<std::pair<std::string, std::string>> ins_list{
	{"MCNC", "apte"},{"MCNC", "xerox"},{"MCNC", "hp"},{"MCNC", "ami33"},{"MCNC", "ami49"},
	{"GSRC", "n10"},{"GSRC", "n30"},{"GSRC", "n50"},{"GSRC", "n100"},{"GSRC", "n200"},{"GSRC", "n300"}
};

static std::unordered_map<std::string, std::pair<int, int>> obj_map{
	{"apte", {47050000, 246000}}, {"xerox", {20340000, 379600}}, {"hp", {9460000, 149800}},
	{"ami33", {1220000, 59500}}, {"ami49", {37820000, 667000}},
	{"n10", {225242, 25788}}, {"n30", {216051, 79740}}, {"n50", {204295, 124326}},
	{"n100", {187880, 206269}}, {"n200", {183931, 389272}}, {"n300", {288702, 587739}}
};

/// �㷨��������
struct Config {
	unsigned int random_seed = std::random_device{}(); // ������ӣ�release�汾ʹ��`random_device{}()`

	double alpha = 0.5, beta = 0.5;        // ����������߳�Ȩ��
	double lb_scale = 0.8, ub_scale = 1.2; // ���ƺ�ѡ�����Ŀ�������

	int ub_time = 3600; // ASA��ʱʱ��
	int ub_iter = 8192; // RLS���������� or BS��������

	enum class LevelCandidateWidth {
		CombRotate, // [deprecated] ������ϼ���ת���������
		CombShort,  // [deprecated] ���Ƕ̱ߵ����
		Interval,   // �Լ���Ⱦ໮�ֿ������
		Sqrt        // ����ƽ�������Ƴ����
	} level_asa_cw = LevelCandidateWidth::Interval;

	enum class LevelFloorplanPacker {
		RandomLocalSearch,
		BeamSearch,
	} level_asa_fbp = LevelFloorplanPacker::BeamSearch;

	enum class LevelWireLength {
		Block,           // ����block�ڲ������߳�
		BlockAndTerminal // ����block��terminal�����߳�
	} level_fbp_wl = LevelWireLength::Block;

	// EDAthon2020P4Ŀ�꺯����(Af+D)/Ac��https://blog.csdn.net/nobleman__/article/details/107880261
	enum class LevelObjDist {
		WireLengthDist,   // �߳�
		SqrEuclideanDist, // ���ζ�֮��ŷʽƽ������ĺ�
		SqrManhattanDist  // ���ζ�֮��������ƽ������ĺ�
	} level_fbp_dist = LevelObjDist::SqrManhattanDist;
} cfg;

std::ostream& operator<<(std::ostream& os, const Config& cfg) {
	switch (cfg.level_asa_fbp) {
	case Config::LevelFloorplanPacker::RandomLocalSearch: os << "RandomLocalSearch,"; break;
	case Config::LevelFloorplanPacker::BeamSearch: os << "BeamSearch,"; break;
	default: break;
	}

	switch (cfg.level_fbp_wl) {
	case Config::LevelWireLength::Block: os << "Block,"; break;
	case Config::LevelWireLength::BlockAndTerminal: os << "BlockAndTerminal,"; break;
	default: break;
	}

	switch (cfg.level_fbp_dist) {
	case Config::LevelObjDist::WireLengthDist: os << "WireLengthDist"; break;
	case Config::LevelObjDist::SqrEuclideanDist: os << "SqrEuclideanDist"; break;
	case Config::LevelObjDist::SqrManhattanDist: os << "SqrManhattanDist"; break;
	default: break;
	}

	return os;
}
