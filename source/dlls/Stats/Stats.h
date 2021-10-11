#ifndef STATS_H
#define STATS_H

typedef unsigned long ulong;

class CSubStat {
public:
	CSubStat( int iValue, int iExp ) { Value = iValue; Exp = iExp; }
	CSubStat( ) { Clear( ); }
	~CSubStat( );
	void Clear( ) { Value = 0; Exp = 0; }
	CSubStat &operator = ( const CSubStat &Other );
	//int Value;  //Level of the skill
	//ulong Exp;  //Total Experience in this skill 
	safevar( int, Value );  //Level of the skill
	safevar( ulong, Exp );  //Total Experience in this skill
	int m_OldValue;
	ulong m_OldExp;
	bool Changed( );
};
class CStat {
public:
	mslist<CSubStat> m_SubStats;	//For regular stats, there are three - Speed, balance, power
									//For spellcasting, there is more
									//For parry, there is only one
	string_i m_Name;
	enum skilltype_e { STAT_NAT, STAT_SKILL } m_Type;	//Stat type

	CStat( ) { } 
	CStat( msstring_ref Name, skilltype_e Type ) { m_Name = Name; m_Type = Type; } 
	bool operator == ( CStat &CompareStat );
	operator int( ) { return Value(); }
	int operator = ( int Equals );
	int operator += ( int Add );
	int Value( );
	int Value( int StatProperty );
	bool operator != ( const CStat &Other );
	bool Changed ( );	//Has changed since OutDate called
	void OutDate( );	//Makes sure a change is sent next frame
	void Update( );		//Sets status to current - No updates sent

	static InitStatList( mslist<CStat> &Stats );
};

typedef mslist<CStat> statlist;
//A caching structure for CStat.  Allows you to do numerous lookups without calling 
//Value() each time.  Used in TitleManager::GetPlayerTitle()
struct skillcache_t
{
	int Skill;
	int Value;
};

struct statinfo_t {
	char *Name;
};
struct skillstatinfo_t {
	const char *Name;
	const char *DllName;
	int StatCount;
};

extern statinfo_t NatStatList[255];
extern skillstatinfo_t SkillStatList[255];
extern char *SkillTypeList[255];
extern char *SpellTypeList[255];

#endif STATS_H