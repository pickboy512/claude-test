// A Yasuo-only League of Legends-style 2D practice tool: last-hit a lane of
// minions and combo a practice dummy using Yasuo's actual kit shape --
// Steel Tempest's 3-stack knockup into Last Breath on an airborne target.
#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sstream>

namespace {

constexpr int kWorldWidth = 1280;
constexpr int kWorldHeight = 800;

constexpr Vector2 kChampionSpawn = {150.0f, 650.0f};
constexpr Vector2 kDummyPos = {220.0f, 150.0f};
constexpr float kLaneY = 420.0f;
constexpr Vector2 kAllySpawn = {60.0f, kLaneY};
constexpr Vector2 kEnemySpawn = {1220.0f, kLaneY};

constexpr float kWaveInterval = 20.0f;
constexpr int kMinionsPerWave = 3;

constexpr float kMinionMaxHp = 150.0f;
constexpr float kMinionAd = 14.0f;
constexpr float kMinionAttackRange = 45.0f;
constexpr float kMinionAttackCooldown = 1.0f;
constexpr float kMinionMoveSpeed = 100.0f;
constexpr float kMinionRadius = 14.0f;
constexpr float kMinionDetectionRange = 450.0f;
constexpr int kMinionGold = 20;
constexpr float kMinionAggroDuration = 4.0f;

constexpr float kChampionMaxHp = 1000.0f;
constexpr float kChampionAd = 65.0f;
constexpr float kChampionAttackRange = 150.0f;
constexpr float kChampionAttackCooldown = 0.85f;
constexpr float kChampionMoveSpeed = 320.0f;
constexpr float kChampionRadius = 20.0f;
constexpr float kChampionRegenPerSec = 8.0f;
constexpr float kChampionRespawnTime = 3.0f;

// Passive: Way of the Wanderer. Moving fills a Flow gauge; at full Flow the
// next hit against Yasuo is absorbed by a shield, then Flow resets.
constexpr float kFlowRatePerSec = 22.0f;
constexpr float kPassiveShieldAmount = 110.0f;

// Q: Steel Tempest. Three casts within the combo window knock the target up
// instead of just damaging it; the ability only goes on full cooldown after
// the 3rd cast (or once the combo window lapses).
constexpr float kQDamage = 90.0f;
constexpr float kQKnockupDamage = 130.0f;
constexpr float kQSpeed = 900.0f;
constexpr float kQRange = 850.0f;
constexpr float kQRadius = 18.0f;
constexpr float kQKnockupRadius = 30.0f;
constexpr float kQRecastDelay = 0.3f;
constexpr float kQCooldownMax = 6.0f;
constexpr float kQComboWindow = 4.0f;
constexpr float kKnockupDuration = 1.1f;

// W: Wind Wall. Purely defensive like the real spell -- no damage, just a
// shield plus a cosmetic wall of wind in front of Yasuo.
constexpr float kWCooldownMax = 12.0f;
constexpr float kWShieldAmount = 130.0f;
constexpr float kWShieldDuration = 3.0f;
constexpr float kWWallLife = 3.0f;
constexpr float kWWallDistance = 70.0f;
constexpr float kWWallHalfLength = 90.0f;

// E: Sweeping Blade. A dash that damages what it passes through; killing
// something with it refunds the cooldown entirely, mirroring the real spell.
constexpr float kEDamage = 55.0f;
constexpr float kECooldownMax = 10.0f;
constexpr float kERange = 320.0f;
constexpr float kERadius = 40.0f;

// R: Last Breath. Only does anything if an airborne enemy is in range --
// dashes to it and deals AoE damage to everything near the impact point.
constexpr float kRDamage = 240.0f;
constexpr float kRCooldownMax = 60.0f;
constexpr float kRRadius = 170.0f;
constexpr float kRCastRange = 650.0f;

constexpr float kDummyMaxHp = 6000.0f;
constexpr float kDummyRadius = 30.0f;
constexpr float kDummyRegenDelay = 2.0f;
constexpr float kDummyRegenTime = 1.5f;

enum class Team { Ally, Enemy };
enum class AttackTargetType { None, Minion, Dummy };

struct Minion {
    int id;
    Vector2 pos;
    Team team;
    float hp;
    float attackTimer;
    float aggroChampionTimer;
    float airborneTimer;
    bool alive;
};

struct Dummy {
    Vector2 pos = kDummyPos;
    float hp = kDummyMaxHp;
    float regenDelayTimer = 0.0f;
    float comboDamage = 0.0f;
    float lastComboDamage = 0.0f;
    float airborneTimer = 0.0f;
    bool wasBelowMax = false;
};

struct Projectile {
    Vector2 pos;
    Vector2 vel;
    float traveled = 0.0f;
    bool alive = true;
    bool knockup = false;
    float radius = kQRadius;
    float damage = kQDamage;
};

struct Wall {
    Vector2 a, b;
    float life;
    float maxLife;
};

struct EffectCircle {
    Vector2 pos;
    float radius;
    float life;
    float maxLife;
    Color color;
};

struct Trail {
    Vector2 start, end;
    float life;
    float maxLife;
};

struct FloatingText {
    Vector2 pos;
    std::string text;
    Color color;
    float life;
    float maxLife;
};

struct Champion {
    Vector2 pos = kChampionSpawn;
    float hp = kChampionMaxHp;
    bool hasMoveTarget = false;
    Vector2 moveTarget{};
    AttackTargetType attackTargetType = AttackTargetType::None;
    int attackTargetId = -1;  // stable Minion::id, not a vector index (which shifts on erase)
    float attackTimer = 0.0f;
    float qCooldown = 0.0f, wCooldown = 0.0f, eCooldown = 0.0f, rCooldown = 0.0f;
    int qStacks = 0;
    float qComboTimer = 0.0f;
    float flow = 0.0f;
    bool shieldActive = false;
    float shieldAmount = 0.0f;
    float respawnTimer = 0.0f;
};

Champion g_champion;
std::vector<Minion> g_minions;
Dummy g_dummy;
std::vector<Projectile> g_projectiles;
std::vector<Wall> g_walls;
std::vector<EffectCircle> g_effects;
std::vector<Trail> g_trails;
std::vector<FloatingText> g_floatingTexts;

float g_waveTimer = kWaveInterval;
int g_cs = 0;
int g_gold = 0;
int g_nextMinionId = 1;

void addFloatingText(Vector2 pos, const std::string& text, Color color) {
    g_floatingTexts.push_back({pos, text, color, 1.0f, 1.0f});
}

void addEffect(Vector2 pos, float radius, float life, Color color) {
    g_effects.push_back({pos, radius, life, life, color});
}

float distancePointToSegment(Vector2 p, Vector2 a, Vector2 b) {
    Vector2 ab = Vector2Subtract(b, a);
    float lenSq = Vector2LengthSqr(ab);
    float t = (lenSq > 0.0001f) ? Clamp(Vector2DotProduct(Vector2Subtract(p, a), ab) / lenSq, 0.0f, 1.0f) : 0.0f;
    Vector2 closest = Vector2Add(a, Vector2Scale(ab, t));
    return Vector2Distance(p, closest);
}

void resetPractice() {
    g_champion = Champion{};
    g_minions.clear();
    g_dummy = Dummy{};
    g_projectiles.clear();
    g_walls.clear();
    g_effects.clear();
    g_trails.clear();
    g_floatingTexts.clear();
    g_waveTimer = kWaveInterval;
    g_cs = 0;
    g_gold = 0;
    g_nextMinionId = 1;
}

void spawnWave() {
    for (int i = 0; i < kMinionsPerWave; ++i) {
        float offset = static_cast<float>(i) * 40.0f;
        float jitter = static_cast<float>(GetRandomValue(-20, 20));
        g_minions.push_back({g_nextMinionId++, {kAllySpawn.x + offset, kLaneY + jitter}, Team::Ally, kMinionMaxHp, 0.0f, 0.0f, 0.0f, true});
        g_minions.push_back({g_nextMinionId++, {kEnemySpawn.x - offset, kLaneY + jitter}, Team::Enemy, kMinionMaxHp, 0.0f, 0.0f, 0.0f, true});
    }
}

// Applies damage to a minion, crediting the player with CS/gold only when
// the champion itself lands the killing blow (mirrors LoL last-hitting).
// Returns true if this hit killed the minion.
bool damageMinion(Minion& m, float dmg, bool fromChampion) {
    if (!m.alive) return false;
    m.hp -= dmg;
    addFloatingText(m.pos, TextFormat("-%.0f", dmg), fromChampion ? YELLOW : WHITE);
    if (fromChampion) m.aggroChampionTimer = kMinionAggroDuration;
    if (m.hp <= 0.0f) {
        m.alive = false;
        if (fromChampion && m.team == Team::Enemy) {
            g_cs++;
            g_gold += kMinionGold;
            addFloatingText(m.pos, TextFormat("+%dg", kMinionGold), GOLD);
        }
        return true;
    }
    return false;
}

void damageDummy(float dmg) {
    g_dummy.hp = Clamp(g_dummy.hp - dmg, 0.0f, kDummyMaxHp);
    g_dummy.comboDamage += dmg;
    g_dummy.regenDelayTimer = kDummyRegenDelay;
    g_dummy.wasBelowMax = true;
    addFloatingText(g_dummy.pos, TextFormat("-%.0f", dmg), ORANGE);
}

void damageChampion(float dmg) {
    if (g_champion.respawnTimer > 0.0f) return;
    if (g_champion.shieldActive) {
        float absorbed = fminf(dmg, g_champion.shieldAmount);
        g_champion.shieldAmount -= absorbed;
        dmg -= absorbed;
        if (absorbed > 0.0f) addFloatingText(g_champion.pos, "Blocked", SKYBLUE);
        if (g_champion.shieldAmount <= 0.0f) g_champion.shieldActive = false;
    }
    if (dmg <= 0.0f) return;
    g_champion.hp -= dmg;
    addFloatingText(g_champion.pos, TextFormat("-%.0f", dmg), RED);
    if (g_champion.hp <= 0.0f) {
        g_champion.hp = 0.0f;
        g_champion.respawnTimer = kChampionRespawnTime;
        g_champion.hasMoveTarget = false;
        g_champion.attackTargetType = AttackTargetType::None;
    }
}

// No camera transform is used, so screen space and world space are the same.
Vector2 GetMouseWorldPos() { return GetMousePosition(); }

bool findNearestAirborneTarget(Vector2 from, float maxRange, Vector2* outPos, Minion** outMinion, bool* outIsDummy) {
    float bestDist = maxRange;
    bool found = false;
    *outMinion = nullptr;
    *outIsDummy = false;
    for (auto& m : g_minions) {
        if (!m.alive || m.team != Team::Enemy || m.airborneTimer <= 0.0f) continue;
        float d = Vector2Distance(from, m.pos);
        if (d < bestDist) {
            bestDist = d;
            *outPos = m.pos;
            *outMinion = &m;
            *outIsDummy = false;
            found = true;
        }
    }
    if (g_dummy.airborneTimer > 0.0f) {
        float d = Vector2Distance(from, g_dummy.pos);
        if (d < bestDist) {
            *outPos = g_dummy.pos;
            *outMinion = nullptr;
            *outIsDummy = true;
            found = true;
        }
    }
    return found;
}

// Right-click move/attack-move: attack the unit under the cursor, or move to
// the clicked ground point if nothing is there.
void issueOrderAt(Vector2 mouse) {
    bool hitUnit = false;
    for (auto& m : g_minions) {
        if (!m.alive || m.team != Team::Enemy) continue;
        if (Vector2Distance(mouse, m.pos) <= kMinionRadius + 8.0f) {
            g_champion.attackTargetType = AttackTargetType::Minion;
            g_champion.attackTargetId = m.id;
            g_champion.hasMoveTarget = false;
            hitUnit = true;
            break;
        }
    }
    if (!hitUnit && Vector2Distance(mouse, g_dummy.pos) <= kDummyRadius + 8.0f) {
        g_champion.attackTargetType = AttackTargetType::Dummy;
        g_champion.hasMoveTarget = false;
        hitUnit = true;
    }
    if (!hitUnit) {
        g_champion.attackTargetType = AttackTargetType::None;
        g_champion.hasMoveTarget = true;
        g_champion.moveTarget = mouse;
    }
}

void handleInput() {
    if (IsKeyPressed(KEY_BACKSPACE)) { resetPractice(); return; }
    if (g_champion.respawnTimer > 0.0f) return;

    if (IsKeyPressed(KEY_S)) {
        g_champion.hasMoveTarget = false;
        g_champion.attackTargetType = AttackTargetType::None;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        issueOrderAt(GetMouseWorldPos());
    }

    Vector2 mouse = GetMouseWorldPos();
    Vector2 aimDir = Vector2Subtract(mouse, g_champion.pos);
    if (Vector2LengthSqr(aimDir) < 0.0001f) aimDir = {1.0f, 0.0f};
    aimDir = Vector2Normalize(aimDir);

    if (IsKeyPressed(KEY_Q) && g_champion.qCooldown <= 0.0f) {
        g_champion.qStacks++;
        bool knockup = g_champion.qStacks >= 3;
        Projectile p;
        p.pos = g_champion.pos;
        p.vel = Vector2Scale(aimDir, knockup ? kQSpeed * 0.75f : kQSpeed);
        p.knockup = knockup;
        p.radius = knockup ? kQKnockupRadius : kQRadius;
        p.damage = knockup ? kQKnockupDamage : kQDamage;
        g_projectiles.push_back(p);
        if (knockup) {
            g_champion.qStacks = 0;
            g_champion.qCooldown = kQCooldownMax;
        } else {
            g_champion.qCooldown = kQRecastDelay;
            g_champion.qComboTimer = kQComboWindow;
        }
    }
    if (IsKeyPressed(KEY_W) && g_champion.wCooldown <= 0.0f) {
        g_champion.shieldActive = true;
        g_champion.shieldAmount = kWShieldAmount;
        Vector2 side = {-aimDir.y, aimDir.x};
        Vector2 center = Vector2Add(g_champion.pos, Vector2Scale(aimDir, kWWallDistance));
        g_walls.push_back({Vector2Subtract(center, Vector2Scale(side, kWWallHalfLength)),
                            Vector2Add(center, Vector2Scale(side, kWWallHalfLength)),
                            kWWallLife, kWWallLife});
        addFloatingText(g_champion.pos, "Shield!", SKYBLUE);
        g_champion.wCooldown = kWCooldownMax;
    }
    if (IsKeyPressed(KEY_E) && g_champion.eCooldown <= 0.0f) {
        float dashDist = fminf(kERange, Vector2Distance(mouse, g_champion.pos));
        Vector2 start = g_champion.pos;
        Vector2 end = Vector2Add(start, Vector2Scale(aimDir, dashDist));
        bool killedSomething = false;
        for (auto& m : g_minions) {
            if (m.alive && m.team == Team::Enemy &&
                distancePointToSegment(m.pos, start, end) <= kERadius) {
                if (damageMinion(m, kEDamage, true)) killedSomething = true;
            }
        }
        if (distancePointToSegment(g_dummy.pos, start, end) <= kERadius) damageDummy(kEDamage);
        g_champion.pos = end;
        g_trails.push_back({start, end, 0.25f, 0.25f});
        g_champion.eCooldown = killedSomething ? 0.0f : kECooldownMax;
        if (killedSomething) addFloatingText(end, "Refunded!", Color{255, 200, 100, 255});
    }
    if (IsKeyPressed(KEY_R) && g_champion.rCooldown <= 0.0f) {
        Vector2 targetPos{};
        Minion* targetMinion = nullptr;
        bool isDummy = false;
        if (findNearestAirborneTarget(g_champion.pos, kRCastRange, &targetPos, &targetMinion, &isDummy)) {
            Vector2 dashTo = Vector2Subtract(targetPos, Vector2Scale(Vector2Normalize(Vector2Subtract(targetPos, g_champion.pos)), kChampionRadius + 10.0f));
            g_trails.push_back({g_champion.pos, dashTo, 0.2f, 0.2f});
            g_champion.pos = dashTo;
            for (auto& m : g_minions) {
                if (m.alive && m.team == Team::Enemy && Vector2Distance(m.pos, targetPos) <= kRRadius) {
                    damageMinion(m, kRDamage, true);
                    m.airborneTimer = fmaxf(m.airborneTimer, 0.6f);
                }
            }
            if (Vector2Distance(g_dummy.pos, targetPos) <= kRRadius) {
                damageDummy(kRDamage);
                g_dummy.airborneTimer = fmaxf(g_dummy.airborneTimer, 0.6f);
            }
            addEffect(targetPos, kRRadius, 0.4f, Fade(PURPLE, 0.6f));
            (void)isDummy;
        } else {
            addFloatingText(g_champion.pos, "No airborne target!", Color{255, 120, 120, 255});
        }
        g_champion.rCooldown = kRCooldownMax;
    }
}

void updateChampion(float dt) {
    if (g_champion.respawnTimer > 0.0f) {
        g_champion.respawnTimer -= dt;
        if (g_champion.respawnTimer <= 0.0f) {
            g_champion.respawnTimer = 0.0f;
            g_champion.hp = kChampionMaxHp;
            g_champion.pos = kChampionSpawn;
        }
        return;
    }

    g_champion.hp = fminf(kChampionMaxHp, g_champion.hp + kChampionRegenPerSec * dt);
    g_champion.qCooldown = fmaxf(0.0f, g_champion.qCooldown - dt);
    g_champion.wCooldown = fmaxf(0.0f, g_champion.wCooldown - dt);
    g_champion.eCooldown = fmaxf(0.0f, g_champion.eCooldown - dt);
    g_champion.rCooldown = fmaxf(0.0f, g_champion.rCooldown - dt);
    g_champion.attackTimer = fmaxf(0.0f, g_champion.attackTimer - dt);

    if (g_champion.qStacks > 0) {
        g_champion.qComboTimer -= dt;
        if (g_champion.qComboTimer <= 0.0f) g_champion.qStacks = 0;
    }

    auto stepToward = [&](Vector2 target) {
        Vector2 dir = Vector2Normalize(Vector2Subtract(target, g_champion.pos));
        g_champion.pos = Vector2Add(g_champion.pos, Vector2Scale(dir, kChampionMoveSpeed * dt));
        g_champion.flow = fminf(100.0f, g_champion.flow + kFlowRatePerSec * dt);
    };

    Vector2* targetPos = nullptr;
    Minion* targetMinion = nullptr;
    if (g_champion.attackTargetType == AttackTargetType::Minion) {
        for (auto& m : g_minions) {
            if (m.alive && m.id == g_champion.attackTargetId) {
                targetMinion = &m;
                break;
            }
        }
        if (targetMinion != nullptr) {
            targetPos = &targetMinion->pos;
        } else {
            g_champion.attackTargetType = AttackTargetType::None;
        }
    } else if (g_champion.attackTargetType == AttackTargetType::Dummy) {
        targetPos = &g_dummy.pos;
    }

    if (targetPos != nullptr) {
        float dist = Vector2Distance(g_champion.pos, *targetPos);
        if (dist <= kChampionAttackRange) {
            if (g_champion.attackTimer <= 0.0f) {
                if (targetMinion != nullptr) damageMinion(*targetMinion, kChampionAd, true);
                else damageDummy(kChampionAd);
                g_champion.attackTimer = kChampionAttackCooldown;
            }
        } else {
            stepToward(*targetPos);
        }
    } else if (g_champion.hasMoveTarget) {
        float dist = Vector2Distance(g_champion.pos, g_champion.moveTarget);
        if (dist <= 4.0f) {
            g_champion.hasMoveTarget = false;
        } else {
            stepToward(g_champion.moveTarget);
        }
    }

    if (g_champion.flow >= 100.0f && !g_champion.shieldActive) {
        g_champion.shieldActive = true;
        g_champion.shieldAmount = kPassiveShieldAmount;
        g_champion.flow = 0.0f;
        addFloatingText(g_champion.pos, "Flow Shield!", Color{150, 230, 240, 255});
    }

    g_champion.pos.x = Clamp(g_champion.pos.x, kChampionRadius, kWorldWidth - kChampionRadius);
    g_champion.pos.y = Clamp(g_champion.pos.y, kChampionRadius, kWorldHeight - kChampionRadius);
}

void updateMinions(float dt) {
    for (auto& m : g_minions) {
        if (!m.alive) continue;
        if (m.airborneTimer > 0.0f) {
            m.airborneTimer -= dt;
            continue;
        }
        m.attackTimer = fmaxf(0.0f, m.attackTimer - dt);
        m.aggroChampionTimer = fmaxf(0.0f, m.aggroChampionTimer - dt);

        bool targetIsChampion = false;
        Vector2 targetPos{};
        bool hasTarget = false;
        Minion* enemyTarget = nullptr;

        if (m.aggroChampionTimer > 0.0f && g_champion.respawnTimer <= 0.0f) {
            targetIsChampion = true;
            targetPos = g_champion.pos;
            hasTarget = true;
        } else {
            float bestDist = kMinionDetectionRange;
            for (auto& other : g_minions) {
                if (!other.alive || other.team == m.team) continue;
                float d = Vector2Distance(m.pos, other.pos);
                if (d < bestDist) {
                    bestDist = d;
                    enemyTarget = &other;
                    targetPos = other.pos;
                    hasTarget = true;
                }
            }
        }

        if (hasTarget) {
            float dist = Vector2Distance(m.pos, targetPos);
            if (dist <= kMinionAttackRange) {
                if (m.attackTimer <= 0.0f) {
                    if (targetIsChampion) damageChampion(kMinionAd);
                    else if (enemyTarget != nullptr) damageMinion(*enemyTarget, kMinionAd, false);
                    m.attackTimer = kMinionAttackCooldown;
                }
            } else {
                Vector2 dir = Vector2Normalize(Vector2Subtract(targetPos, m.pos));
                m.pos = Vector2Add(m.pos, Vector2Scale(dir, kMinionMoveSpeed * dt));
            }
        } else {
            float dirX = (m.team == Team::Ally) ? 1.0f : -1.0f;
            m.pos.x += dirX * kMinionMoveSpeed * dt;
            m.pos.y = Lerp(m.pos.y, kLaneY, dt * 2.0f);
        }
    }

    for (auto& m : g_minions) {
        if (m.hp <= 0.0f) m.alive = false;
    }
    g_minions.erase(std::remove_if(g_minions.begin(), g_minions.end(),
                                    [](const Minion& m) { return !m.alive; }),
                     g_minions.end());
}

void updateDummy(float dt) {
    if (g_dummy.airborneTimer > 0.0f) g_dummy.airborneTimer -= dt;
    if (g_dummy.regenDelayTimer > 0.0f) {
        g_dummy.regenDelayTimer -= dt;
    } else if (g_dummy.hp < kDummyMaxHp) {
        g_dummy.hp = fminf(kDummyMaxHp, g_dummy.hp + (kDummyMaxHp / kDummyRegenTime) * dt);
        if (g_dummy.hp >= kDummyMaxHp && g_dummy.wasBelowMax) {
            g_dummy.lastComboDamage = g_dummy.comboDamage;
            g_dummy.comboDamage = 0.0f;
            g_dummy.wasBelowMax = false;
        }
    }
}

void updateProjectiles(float dt) {
    for (auto& p : g_projectiles) {
        if (!p.alive) continue;
        p.pos = Vector2Add(p.pos, Vector2Scale(p.vel, dt));
        p.traveled += Vector2Length(p.vel) * dt;

        bool hit = false;
        for (auto& m : g_minions) {
            if (!m.alive || m.team != Team::Enemy) continue;
            if (Vector2Distance(p.pos, m.pos) <= p.radius + kMinionRadius) {
                damageMinion(m, p.damage, true);
                if (p.knockup) m.airborneTimer = kKnockupDuration;
                hit = true;
                break;
            }
        }
        if (!hit && Vector2Distance(p.pos, g_dummy.pos) <= p.radius + kDummyRadius) {
            damageDummy(p.damage);
            if (p.knockup) g_dummy.airborneTimer = kKnockupDuration;
            hit = true;
        }
        if (hit || p.traveled >= kQRange ||
            p.pos.x < 0 || p.pos.x > kWorldWidth || p.pos.y < 0 || p.pos.y > kWorldHeight) {
            p.alive = false;
        }
    }
    g_projectiles.erase(std::remove_if(g_projectiles.begin(), g_projectiles.end(),
                                        [](const Projectile& p) { return !p.alive; }),
                         g_projectiles.end());
}

void updateEffects(float dt) {
    for (auto& e : g_effects) e.life -= dt;
    g_effects.erase(std::remove_if(g_effects.begin(), g_effects.end(),
                                    [](const EffectCircle& e) { return e.life <= 0.0f; }),
                     g_effects.end());
    for (auto& t : g_trails) t.life -= dt;
    g_trails.erase(std::remove_if(g_trails.begin(), g_trails.end(),
                                   [](const Trail& t) { return t.life <= 0.0f; }),
                    g_trails.end());
    for (auto& w : g_walls) w.life -= dt;
    g_walls.erase(std::remove_if(g_walls.begin(), g_walls.end(),
                                  [](const Wall& w) { return w.life <= 0.0f; }),
                   g_walls.end());
    for (auto& f : g_floatingTexts) {
        f.life -= dt;
        f.pos.y -= 40.0f * dt;
    }
    g_floatingTexts.erase(std::remove_if(g_floatingTexts.begin(), g_floatingTexts.end(),
                                          [](const FloatingText& f) { return f.life <= 0.0f; }),
                           g_floatingTexts.end());
}

void update(float dt) {
    handleInput();
    updateChampion(dt);
    updateMinions(dt);
    updateDummy(dt);
    updateProjectiles(dt);
    updateEffects(dt);

    g_waveTimer -= dt;
    if (g_waveTimer <= 0.0f) {
        spawnWave();
        g_waveTimer = kWaveInterval;
    }
}

void drawHealthBar(Vector2 pos, float width, float height, float hp, float maxHp, Color fillColor) {
    Rectangle back{pos.x - width / 2.0f, pos.y, width, height};
    DrawRectangleRec(back, Fade(BLACK, 0.6f));
    float frac = Clamp(hp / maxHp, 0.0f, 1.0f);
    DrawRectangleRec({back.x, back.y, width * frac, height}, fillColor);
    DrawRectangleLinesEx(back, 1.0f, Fade(BLACK, 0.8f));
}

// Airborne units are drawn lifted off the ground with a shadow left behind,
// so a knock-up reads clearly at a glance.
Vector2 airborneLift(Vector2 pos, float airborneTimer, float maxDuration) {
    if (airborneTimer <= 0.0f) return pos;
    float t = airborneTimer / maxDuration;
    float lift = 22.0f * sinf(t * 3.14159f);
    return {pos.x, pos.y - lift};
}

void drawWorld() {
    DrawRectangle(0, 0, kWorldWidth, kWorldHeight, Color{28, 46, 30, 255});
    DrawRectangle(0, static_cast<int>(kLaneY) - 45, kWorldWidth, 90, Color{58, 50, 38, 255});
    DrawCircleV(kAllySpawn, 55.0f, Fade(SKYBLUE, 0.35f));
    DrawCircleV(kEnemySpawn, 55.0f, Fade(RED, 0.35f));

    // Practice dummy.
    DrawEllipse(static_cast<int>(g_dummy.pos.x), static_cast<int>(g_dummy.pos.y), kDummyRadius * 0.8f, 8.0f, Fade(BLACK, 0.3f));
    Vector2 dummyDraw = airborneLift(g_dummy.pos, g_dummy.airborneTimer, kKnockupDuration);
    DrawCircleV(dummyDraw, kDummyRadius, Color{120, 80, 40, 255});
    DrawCircleLines(static_cast<int>(dummyDraw.x), static_cast<int>(dummyDraw.y),
                     kDummyRadius, Color{80, 50, 20, 255});
    drawHealthBar({g_dummy.pos.x, g_dummy.pos.y - kDummyRadius - 16.0f}, 80.0f, 8.0f,
                   g_dummy.hp, kDummyMaxHp, ORANGE);

    for (auto& m : g_minions) {
        Color body = (m.team == Team::Ally) ? Color{80, 150, 230, 255} : Color{220, 70, 70, 255};
        DrawEllipse(static_cast<int>(m.pos.x), static_cast<int>(m.pos.y), kMinionRadius * 0.8f, 6.0f, Fade(BLACK, 0.3f));
        Vector2 drawPos = airborneLift(m.pos, m.airborneTimer, kKnockupDuration);
        DrawCircleV(drawPos, kMinionRadius, body);
        drawHealthBar({m.pos.x, m.pos.y - kMinionRadius - 12.0f}, 32.0f, 5.0f, m.hp, kMinionMaxHp,
                       (m.team == Team::Ally) ? SKYBLUE : Color{255, 90, 90, 255});
    }

    for (auto& e : g_effects) {
        float t = e.life / e.maxLife;
        DrawCircleV(e.pos, e.radius * (1.0f - 0.3f * t), Fade(e.color, 0.5f * t));
    }
    for (auto& tr : g_trails) {
        float alpha = tr.life / tr.maxLife;
        DrawLineEx(tr.start, tr.end, 6.0f, Fade(SKYBLUE, alpha));
    }
    for (auto& w : g_walls) {
        float alpha = w.life / w.maxLife;
        DrawLineEx(w.a, w.b, 10.0f, Fade(Color{220, 240, 255, 255}, 0.55f * alpha));
    }
    for (auto& p : g_projectiles) {
        Color c = p.knockup ? Fade(Color{230, 240, 255, 255}, 0.9f) : Fade(Color{200, 230, 235, 255}, 0.85f);
        DrawCircleV(p.pos, p.radius, c);
        DrawCircleLines(static_cast<int>(p.pos.x), static_cast<int>(p.pos.y), p.radius, Fade(WHITE, 0.7f));
    }

    // Champion.
    if (g_champion.respawnTimer <= 0.0f) {
        DrawCircleV(g_champion.pos, kChampionRadius, Color{225, 225, 235, 255});
        DrawCircleLines(static_cast<int>(g_champion.pos.x), static_cast<int>(g_champion.pos.y),
                         kChampionRadius, Color{70, 170, 210, 255});
        if (g_champion.shieldActive) {
            DrawCircleLines(static_cast<int>(g_champion.pos.x), static_cast<int>(g_champion.pos.y),
                             kChampionRadius + 5.0f, Fade(SKYBLUE, 0.8f));
        }
        drawHealthBar({g_champion.pos.x, g_champion.pos.y - kChampionRadius - 16.0f}, 50.0f, 7.0f,
                       g_champion.hp, kChampionMaxHp, LIME);
        if (g_champion.hasMoveTarget) {
            DrawCircleLines(static_cast<int>(g_champion.moveTarget.x),
                             static_cast<int>(g_champion.moveTarget.y), 8.0f, Fade(WHITE, 0.6f));
        }
    } else {
        DrawText(TextFormat("Respawning in %.1f", g_champion.respawnTimer),
                  static_cast<int>(g_champion.pos.x) - 60, static_cast<int>(g_champion.pos.y), 18, RED);
    }

    for (auto& f : g_floatingTexts) {
        int w = MeasureText(f.text.c_str(), 18);
        DrawText(f.text.c_str(), static_cast<int>(f.pos.x) - w / 2, static_cast<int>(f.pos.y), 18,
                  Fade(f.color, f.life / f.maxLife));
    }
}

void drawAbilityIcon(int x, int y, const char* label, float cooldown, float maxCooldown, Color color, int stacks) {
    Rectangle box{static_cast<float>(x), static_cast<float>(y), 54.0f, 54.0f};
    DrawRectangleRec(box, color);
    DrawRectangleLinesEx(box, 2.0f, BLACK);
    int tw = MeasureText(label, 20);
    DrawText(label, x + 27 - tw / 2, y + 4, 20, BLACK);
    if (stacks > 0) {
        for (int i = 0; i < stacks; ++i) {
            DrawCircle(x + 10 + i * 12, y + 48, 4.0f, BLACK);
        }
    }
    if (cooldown > 0.0f) {
        float frac = cooldown / maxCooldown;
        DrawRectangleRec({box.x, box.y + box.height * (1.0f - frac), box.width, box.height * frac},
                          Fade(BLACK, 0.65f));
        std::ostringstream ss;
        ss.precision(1);
        ss << std::fixed << cooldown;
        int cw = MeasureText(ss.str().c_str(), 18);
        DrawText(ss.str().c_str(), x + 27 - cw / 2, y + 30, 18, RAYWHITE);
    }
}

void drawHud() {
    DrawRectangle(0, kWorldHeight - 96, kWorldWidth, 96, Color{15, 15, 20, 230});

    drawHealthBar({150.0f, static_cast<float>(kWorldHeight - 76)}, 260.0f, 18.0f,
                   g_champion.hp, kChampionMaxHp, LIME);
    DrawText(TextFormat("HP %.0f / %.0f", g_champion.hp, kChampionMaxHp), 30, kWorldHeight - 74, 14, RAYWHITE);
    drawHealthBar({150.0f, static_cast<float>(kWorldHeight - 52)}, 260.0f, 10.0f,
                   g_champion.flow, 100.0f, Color{150, 230, 240, 255});
    DrawText(g_champion.shieldActive ? TextFormat("Flow | Shield %.0f", g_champion.shieldAmount) : "Flow",
              30, kWorldHeight - 50, 12, Color{150, 230, 240, 255});

    drawAbilityIcon(320, kWorldHeight - 84, "Q", g_champion.qCooldown, kQCooldownMax,
                     Color{100, 180, 255, 255}, g_champion.qStacks);
    drawAbilityIcon(380, kWorldHeight - 84, "W", g_champion.wCooldown, kWCooldownMax, Color{200, 220, 230, 255}, 0);
    drawAbilityIcon(440, kWorldHeight - 84, "E", g_champion.eCooldown, kECooldownMax, Color{255, 200, 100, 255}, 0);
    drawAbilityIcon(500, kWorldHeight - 84, "R", g_champion.rCooldown, kRCooldownMax, Color{220, 120, 255, 255}, 0);

    DrawText("Q Steel Tempest   W Wind Wall   E Sweeping Blade   R Last Breath",
              580, kWorldHeight - 84, 14, GRAY);
    DrawText("RMB move/attack   Qx3=knock up   E kill=free recast   R=execute airborne   S=stop   Backspace=reset",
              580, kWorldHeight - 40, 12, GRAY);

    DrawText(TextFormat("CS: %d   Gold: %dg", g_cs, g_gold), 10, 10, 22, GOLD);
    DrawText(TextFormat("Next wave: %.0fs", g_waveTimer), kWorldWidth / 2 - 70, 10, 20, RAYWHITE);
    DrawText(TextFormat("Last combo dmg: %.0f", g_dummy.lastComboDamage), kWorldWidth - 260, 10, 20, ORANGE);
}

}  // namespace

int main() {
    SetRandomSeed(static_cast<unsigned>(time(nullptr)));
    InitWindow(kWorldWidth, kWorldHeight, "Yasuo Practice Tool");
    SetTargetFPS(60);

    resetPractice();

    while (!WindowShouldClose()) {
        update(GetFrameTime());

        BeginDrawing();
        ClearBackground(BLACK);
        drawWorld();
        drawHud();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
