// monsters_data.cpp —— 妖怪图鉴全表（32 种，从野狗妖到守层者）
// 平衡设计：
//   * exp/gold 大致跟 hp+atk 走，深层妖更肥
//   * ai 决定行为：Brave 追得远打得狠，Coward 会逃，Guard 只守房间，Sneaky 闪避高
#include "monster.h"

namespace yaota {

const std::vector<MonsterDef>& monsterDex() {
    static const std::vector<MonsterDef> dex = {
        {  0, L"野狗妖",     L'妖', Element::Jin,   15,  5,  1,   8,   3,  1,  5, AiType::Melee,  L"成精的野狗，只会龇牙，塔底一抓一大把。" },
        {  1, L"树精",       L'妖', Element::Mu,    25,  6,  3,  12,   5,  1,  6, AiType::Guard,   L"扎根在塔里的老树成精，挪不动窝，也别想绕过它。" },
        {  2, L"石魄",       L'妖', Element::Tu,    30,  7,  6,  15,   6,  2,  8, AiType::Guard,   L"一块山岩吸了妖气有了意识，皮糙肉厚。" },
        {  3, L"火鼠",       L'妖', Element::Huo,   18,  9,  2,  14,   7,  2,  7, AiType::Sneaky,  L"浑身冒火的小老鼠，滑不留手，还会偷丹药。" },
        {  4, L"水鬼",       L'鬼', Element::Shui,  22,  8,  3,  15,   8,  3,  9, AiType::Melee,   L"淹死在塔中暗河的冤魂，见人就拖下水。" },
        {  5, L"骷髅兵",     L'骨', Element::Jin,   26, 10,  4,  17,   9,  3, 10, AiType::Melee,   L"生前是守塔的士卒，死后还在巡逻。" },
        {  6, L"鬼火",       L'火', Element::Huo,   12, 11,  0,  16,  10,  4, 10, AiType::Sneaky,  L"幽幽一点绿火，飘忽不定，沾上就烧。" },
        {  7, L"狼妖",       L'妖', Element::Jin,   35, 12,  4,  22,  12,  5, 12, AiType::Brave,   L"塔中狼群的头狼，记仇，一旦结仇追你三层。" },
        {  8, L"毒蛛",       L'妖', Element::Mu,    28, 11,  3,  21,  11,  5, 12, AiType::Sneaky,  L"指甲盖大的毒牙，咬一口连着疼好几下。" },
        {  9, L"食人花",     L'妖', Element::Mu,    45, 13,  5,  26,  14,  6, 13, AiType::Guard,   L"花开艳丽，花下白骨累累。" },
        { 10, L"岩甲龟",     L'妖', Element::Tu,    70, 10, 12,  30,  16,  7, 15, AiType::Guard,   L"缩进壳里的时候，趁早绕道。" },
        { 11, L"火蜥蜴",     L'妖', Element::Huo,   40, 16,  6,  28,  15,  7, 14, AiType::Melee,   L"在岩浆里泡澡的蜥蜴，脾气火爆。" },
        { 12, L"寒冰鲤",     L'妖', Element::Shui,  38, 15,  7,  27,  15,  8, 15, AiType::Melee,   L"离水也能活的冰鲤，尾巴扫过来像鞭子抽。" },
        { 13, L"妖狐",       L'妖', Element::Mu,    50, 17,  6,  34,  20,  9, 16, AiType::Coward,  L"九条尾巴只剩三条，狡猾得很，打不过就跑。" },
        { 14, L"白骨将军",   L'骨', Element::Jin,   60, 18,  8,  40,  24, 10, 18, AiType::Brave,   L"锈迹斑斑的盔甲，枪法却一点没锈。" },
        { 15, L"血蝠",       L'妖', Element::Huo,   45, 17,  4,  33,  18, 11, 18, AiType::Coward,  L"吸饱了血就飞走，饿疯了才回来拼命。" },
        { 16, L"罗刹鬼",     L'鬼', Element::Huo,   55, 20,  6,  38,  22, 10, 18, AiType::Sneaky,  L"笑起来最好看的鬼，出手也最黑。" },
        { 17, L"双头蛇",     L'妖', Element::Huo,   65, 19,  8,  40,  23, 12, 19, AiType::Melee,   L"两个头商量好了一起咬你。" },
        { 18, L"幽冥水母",   L'妖', Element::Shui,  58, 20,  7,  39,  22, 13, 20, AiType::Sneaky,  L"飘在半空，伞盖下拖着长长的毒丝。" },
        { 19, L"黑山老妖",   L'妖', Element::Tu,    90, 18, 12,  48,  30, 12, 20, AiType::Guard,   L"盘踞一方的老妖怪，塔都得给它让地方。" },
        { 20, L"铁背蜈蚣",   L'妖', Element::Jin,   75, 18, 11,  44,  26, 13, 21, AiType::Melee,   L"甲壳比铁还硬，一百多只脚跑得飞快。" },
        { 21, L"灵猿",       L'妖', Element::Mu,    70, 21,  8,  46,  28, 14, 22, AiType::Brave,   L"通人性的白猿，抢你的丹药还会朝你做鬼脸。" },
        { 22, L"玄龟",       L'妖', Element::Shui, 110, 16, 16,  52,  32, 15, 24, AiType::Guard,   L"驮着座小山似的龟壳，慢，但你也打不动它。" },
        { 23, L"赤炎魔",     L'魔', Element::Huo,   85, 26, 10,  55,  36, 16, 25, AiType::Brave,   L"由塔中怨火凝成的魔物，越烧越旺。" },
        { 24, L"剑灵",       L'灵', Element::Jin,   80, 28,  9,  54,  35, 17, 26, AiType::Sneaky,  L"前人遗剑所化之灵，出剑快过你的眼睛。" },
        { 25, L"妖僧",       L'妖', Element::Tu,   100, 22, 14,  58,  40, 18, 27, AiType::Guard,   L"念的是魔经，敲的是妖鼓，度的是它自己。" },
        { 26, L"九婴蛇",     L'妖', Element::Shui, 120, 27, 13,  64,  45, 20, 28, AiType::Melee,   L"九个脑袋轮流咬，断一个还有八个。" },
        { 27, L"雷兽",       L'妖', Element::Jin,   105, 30, 12,  66,  48, 21, 29, AiType::Brave,   L"吼一声就是一道雷，塔顶的雷都是它招的。" },
        { 28, L"修罗",       L'魔', Element::Huo,   130, 33, 14,  74,  55, 23, 30, AiType::Brave,   L"战意化形的杀神，见了它就别想好好说话。" },
        { 29, L"太阴魔君",   L'魔', Element::Shui, 150, 30, 18,  82,  62, 25, 30, AiType::Sneaky,  L"月色越深它越强，塔中三十层的阴影都是它的。" },
        { 30, L"混沌魔兽",   L'魔', Element::Tu,   180, 35, 20,  95,  75, 27, 30, AiType::Melee,   L"天地杂质所化的凶兽，没有形状，只有饿。" },
        { 31, L"妖塔守层者", L'塔', Element::Jin,  200, 38, 22, 120,  99, 28, 30, AiType::Brave,   L"塔灵亲手捏的守卫，杀它等于在拆塔。" },
    };
    return dex;
}

const MonsterDef& monsterDef(int id) {
    for (const auto& d : monsterDex())
        if (d.id == id) return d;
    return monsterDex().front(); // 不该发生
}

} // namespace yaota
