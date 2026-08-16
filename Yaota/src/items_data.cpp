// items_data.cpp —— 物品图鉴全表（法宝 / 法袍 / 丹药 / 卷轴 / 奇物 / 炼材）
// 平衡设计：
//   * 法宝 power = 攻击加成；法袍 power = 防御加成
//   * 丹药 power = 回复量（特殊丹药靠 id 在使用逻辑里特判）
//   * minFloor 控制"越深的层出越好的东西"
#include "item.h"

namespace yaota {

const std::vector<ItemDef>& itemDex() {
    static const std::vector<ItemDef> dex = {
        // ================= 法宝（武器） =================
        {  1, L"铁剑",       L'剑', ItemType::Weapon, Element::Jin,   3,   15,  1, L"凡铁打造，剑刃有缺口，聊胜于无。" },
        {  2, L"青竹杖",     L'杖', ItemType::Weapon, Element::Mu,    4,   18,  1, L"后山砍的青竹，自带三分草木灵气。" },
        {  3, L"火浣鞭",     L'鞭', ItemType::Weapon, Element::Huo,   6,   40,  3, L"鞭梢缠着火浣纱，抽在人身上会烧起来。" },
        {  4, L"玄冰刺",     L'刺', ItemType::Weapon, Element::Shui,  7,   48,  4, L"寒潭底捞出的冰锥，千年不化。" },
        {  5, L"裂山斧",     L'斧', ItemType::Weapon, Element::Tu,    9,   70,  6, L"斧刃过处，山石如豆腐般裂开。" },
        {  6, L"银月双环",   L'环', ItemType::Weapon, Element::Jin,  11,   95,  8, L"一对银环，掷出时如月牙交叠。" },
        {  7, L"缠魂藤",     L'藤', ItemType::Weapon, Element::Mu,   12,  105,  9, L"妖藤炼制，缠住敌人生生吸取精血。" },
        {  8, L"赤霄剑",     L'剑', ItemType::Weapon, Element::Huo,  15,  150, 12, L"剑身赤红，出鞘时有龙吟，传闻沾过龙血。" },
        {  9, L"碧波刃",     L'刃', ItemType::Weapon, Element::Shui, 16,  160, 13, L"薄如蝉翼的水属性飞刃，杀人不见血。" },
        { 10, L"崩岳锤",     L'锤', ItemType::Weapon, Element::Tu,   19,  210, 15, L"一锤落下，方圆丈内地面龟裂。" },
        { 11, L"诛邪剑",     L'剑', ItemType::Weapon, Element::Jin,  24,  300, 18, L"剑格上刻着\"诛邪\"二字，专斩妖魔。" },
        { 12, L"建木枝",     L'枝', ItemType::Weapon, Element::Mu,   26,  330, 20, L"神木建木的一根断枝，隐隐有生机流转。" },
        { 13, L"焚天戟",     L'戟', ItemType::Weapon, Element::Huo,  31,  420, 23, L"传说此戟一出，天都要被烧穿一个洞。" },
        { 14, L"弱水绫",     L'绫', ItemType::Weapon, Element::Shui, 33,  450, 25, L"轻若鸿毛，可弱水三千，触之即沉。" },
        { 15, L"撼地印",     L'印', ItemType::Weapon, Element::Tu,   38,  560, 27, L"上古大能的镇物法印，一印可撼山岳。" },

        // ================= 法袍（护甲） =================
        { 20, L"粗布道袍",   L'袍', ItemType::Armor, Element::Tu,     2,   10,  1, L"洗得发白的旧道袍，胜在透气。" },
        { 21, L"藤甲",       L'甲', ItemType::Armor, Element::Mu,     3,   20,  2, L"山中妖藤编成，轻便结实。" },
        { 22, L"铜鳞软甲",   L'甲', ItemType::Armor, Element::Jin,    5,   45,  5, L"一片片铜鳞缀成，刀剑难透。" },
        { 23, L"水纹长衫",   L'衫', ItemType::Armor, Element::Shui,   7,   70,  8, L"衫上水纹流转，可卸去大力。" },
        { 24, L"火浣衣",     L'衣', ItemType::Armor, Element::Huo,    9,  100, 11, L"入火不焚，越烧越亮。" },
        { 25, L"玄龟甲袍",   L'袍', ItemType::Armor, Element::Shui,  12,  150, 14, L"以千年玄龟腹甲磨制，厚重难破。" },
        { 26, L"金丝仙衣",   L'衣', ItemType::Armor, Element::Jin,   15,  210, 17, L"金丝银线织就，刀枪不入，水火不侵。" },
        { 27, L"万年沉木甲", L'甲', ItemType::Armor, Element::Mu,    18,  280, 20, L"沉万年之木，韧过精钢。" },
        { 28, L"离火法袍",   L'袍', ItemType::Armor, Element::Huo,   22,  380, 24, L"袍上绣着离火符文，妖物近身即灼。" },
        { 29, L"后土宝铠",   L'铠', ItemType::Armor, Element::Tu,    26,  500, 27, L"承后土之德，穿它的人很难被杀死。" },

        // ================= 丹药 =================
        // power = 回复量；特殊效果按 id 特判
        { 40, L"回气散",     L'丹', ItemType::Pill, Element::Mu,    40,   12,  1, L"最粗浅的药散，运气回气聊应急。" },
        { 41, L"小还丹",     L'丹', ItemType::Pill, Element::Mu,   100,   35,  4, L"外伤内损皆可用，散修居家必备。" },
        { 42, L"大还丹",     L'丹', ItemType::Pill, Element::Mu,   260,   90, 12, L"只要还有一口气，一颗下去就能爬起来。" },
        { 43, L"九转续命丹", L'丹', ItemType::Pill, Element::Mu,   600,  260, 20, L"九转丹成，阎王见了都摇头。" },
        { 44, L"聚灵丹",     L'丹', ItemType::Pill, Element::Shui,  40,   60,  6, L"服下后灵气汇聚，修为噌噌上涨。（+40 修为）" },
        { 45, L"培元丹",     L'丹', ItemType::Pill, Element::Tu,    20,   80,  8, L"固本培元，气血上限永久 +20。" },
        { 46, L"洗髓丹",     L'丹', ItemType::Pill, Element::Jin,    2,  120, 10, L"洗经伐髓，攻击永久 +2。" },
        { 47, L"金刚丹",     L'丹', ItemType::Pill, Element::Tu,    2,  120, 10, L"筋骨如金刚，防御永久 +2。" },
        { 48, L"破障丹",     L'丹', ItemType::Pill, Element::Huo,   15,  150,  5, L"冲关破障，下一次渡劫成功率 +15%。" },
        { 49, L"解毒丹",     L'丹', ItemType::Pill, Element::Mu,     0,   25,  2, L"一味解毒的常备药，苦得皱眉。" },
        { 50, L"龟息丹",     L'丹', ItemType::Pill, Element::Shui, 999,  200, 15, L"回复一半上限的气血，气息绵长如龟。" },

        // ================= 卷轴 =================
        // power = 效果强度
        { 60, L"火球符",     L'符', ItemType::Scroll, Element::Huo,  30,   30,  3, L"掷出即炸，身边妖怪统统吃一发火球。" },
        { 61, L"冰封符",     L'符', ItemType::Scroll, Element::Shui,  3,   45,  5, L"寒气扩散，冻住身边的妖怪动弹不得。" },
        { 62, L"传送符",     L'符', ItemType::Scroll, Element::Tu,    0,   25,  2, L"撕开就走，随机传到本层某处。" },
        { 63, L"天眼符",     L'符', ItemType::Scroll, Element::Jin,   0,   40,  4, L"开天眼，本层地图尽收眼底。" },
        { 64, L"摄妖符",     L'符', ItemType::Scroll, Element::Jin,   2,   70,  8, L"符光一闪，视野内的妖怪集体一僵。" },
        { 65, L"五雷符",     L'符', ItemType::Scroll, Element::Jin, 120,  110, 12, L"引天雷入符，劈最近的妖怪一下狠的。" },

        // ================= 奇物（值钱） =================
        { 80, L"妖丹",       L'丹', ItemType::Treasure, Element::Tu,    0,   30,  1, L"妖怪体内凝结的精丹，值点灵石。" },
        { 81, L"千年灵芝",   L'芝', ItemType::Treasure, Element::Mu,    0,   60,  6, L"千年灵物，药香扑鼻，卖个好价钱。" },
        { 82, L"龙鳞",       L'鳞', ItemType::Treasure, Element::Jin,   0,  150, 14, L"真正的龙鳞！大妖见了都要眼红。" },
        { 83, L"凤羽",       L'羽', ItemType::Treasure, Element::Huo,   0,  300, 22, L"凤凰遗羽，握在手里微微发烫。" },
        { 84, L"混沌石",     L'石', ItemType::Treasure, Element::Tu,    0,  600, 26, L"天地初开时的一粒混沌，无价之宝。" },

        // ================= 炼材（卖钱） =================
        { 90, L"灵草",       L'草', ItemType::Material, Element::Mu,    0,   12,  1, L"带着灵气的野草，炼丹的底料。" },
        { 91, L"铁矿石",     L'矿', ItemType::Material, Element::Jin,   0,    8,  1, L"黑黢黢的矿石，炼器坊按斤收。" },
        { 92, L"妖骨",       L'骨', ItemType::Material, Element::Tu,    0,   18,  3, L"妖怪的骨头，硬而不朽。" },
        { 93, L"星辰砂",     L'砂', ItemType::Material, Element::Huo,   0,   45, 10, L"夜里会发光的细砂，据说是星星的碎屑。" },
    };
    return dex;
}

const ItemDef& itemDef(int id) {
    for (const auto& d : itemDex())
        if (d.id == id) return d;
    return itemDex().front(); // 不该发生
}

} // namespace yaota
