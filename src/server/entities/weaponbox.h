
#pragma once

class CBasePlayerAmmo : public CBaseEntity
{
public:
	bool Spawn() override;
	void EXPORT DefaultTouch(CBaseEntity* pOther); // default weapon touch
	virtual bool AddAmmo(CBaseEntity* pOther) { return true; }

	CBaseEntity* Respawn() override;
	void EXPORT Materialize();
};

//=========================================================
// CWeaponBox - a single entity that can store weapons
// and ammo.
//=========================================================
class CWeaponBox : public CBaseEntity
{
public:
	CWeaponBox(Entity* containingEntity) : CBaseEntity(containingEntity) {}
	~CWeaponBox() override;

	DECLARE_SAVERESTORE()

	void Precache() override;
	bool Spawn() override;
	void Touch(CBaseEntity* pOther) override;
	bool KeyValue(KeyValueData* pkvd) override;
	bool IsEmpty();
	void SetObjectCollisionBox() override;

	void RemoveWeapons();

	bool HasWeapon(CBasePlayerWeapon* pCheckWeapon);
	bool PackWeapon(CBasePlayerWeapon* pWeapon);
	bool PackAmmo(int iType, int iCount);

	EHANDLE m_hPlayerWeapons[WEAPON_TYPES]; // one slot for each

	byte m_rgAmmo[AMMO_TYPES];	 // ammo quantities

	int m_cAmmoTypes; // how many ammo types packed into this box (if packed by a level designer)
};
