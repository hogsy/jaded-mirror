
/*$T AIdeffields.h GC!1.22 04/07/99 10:21:05 */

/*
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Aim:    Definition of all fields.

    Note:   ID is <512, 2047> �
            NEVER CHANGE THE ID OF AN EXISTING ITEM !!!!
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

/*$off*/
DEFINE_FIELD(512,   FIELD_VECX,			".x",			TYPE_FLOAT,			TYPE_VECTOR,		AI_EvalField_VecX)
DEFINE_FIELD(513,   FIELD_VECY,			".y",			TYPE_FLOAT,			TYPE_VECTOR,		AI_EvalField_VecY)
DEFINE_FIELD(514,   FIELD_VECZ,			".z",			TYPE_FLOAT,			TYPE_VECTOR,		AI_EvalField_VecZ)

DEFINE_FIELD(515,   FIELD_MSGSENDER,    ".msg_sender",  TYPE_GAMEOBJECT,    TYPE_MESSAGE,		AI_EvalField_MsgSender)
DEFINE_FIELD(516,   FIELD_MSGID,		".msg_id",		TYPE_INT,			TYPE_MESSAGE,		AI_EvalField_MsgId)
DEFINE_FIELD(517,   FIELD_MSGVEC1,		".msg_vec1",	TYPE_VECTOR,		TYPE_MESSAGE,		AI_EvalField_MsgVec1)
DEFINE_FIELD(518,   FIELD_MSGVEC2,		".msg_vec2",	TYPE_VECTOR,		TYPE_MESSAGE,		AI_EvalField_MsgVec2)
DEFINE_FIELD(519,   FIELD_MSGVEC3,		".msg_vec3",	TYPE_VECTOR,		TYPE_MESSAGE,		AI_EvalField_MsgVec3)
DEFINE_FIELD(520,   FIELD_MSGVEC4,		".msg_vec4",	TYPE_VECTOR,		TYPE_MESSAGE,		AI_EvalField_MsgVec4)
DEFINE_FIELD(521,   FIELD_MSGVEC5,		".msg_vec5",	TYPE_VECTOR,		TYPE_MESSAGE,		AI_EvalField_MsgVec5)
DEFINE_FIELD(527,   FIELD_MSGGAO1,		".msg_gao1",	TYPE_GAMEOBJECT,    TYPE_MESSAGE,		AI_EvalField_MsgGAO1)
DEFINE_FIELD(528,   FIELD_MSGGAO2,		".msg_gao2",	TYPE_GAMEOBJECT,    TYPE_MESSAGE,		AI_EvalField_MsgGAO2)
DEFINE_FIELD(529,   FIELD_MSGGAO3,		".msg_gao3",	TYPE_GAMEOBJECT,    TYPE_MESSAGE,		AI_EvalField_MsgGAO3)
DEFINE_FIELD(530,   FIELD_MSGGAO4,		".msg_gao4",	TYPE_GAMEOBJECT,    TYPE_MESSAGE,		AI_EvalField_MsgGAO4)
DEFINE_FIELD(531,   FIELD_MSGGAO5,		".msg_gao5",	TYPE_GAMEOBJECT,    TYPE_MESSAGE,		AI_EvalField_MsgGAO5)
DEFINE_FIELD(532,   FIELD_MSGINT1,		".msg_int1",	TYPE_INT,			TYPE_MESSAGE,		AI_EvalField_MsgInt1)
DEFINE_FIELD(533,   FIELD_MSGINT2,		".msg_int2",	TYPE_INT,			TYPE_MESSAGE,		AI_EvalField_MsgInt2)
DEFINE_FIELD(534,   FIELD_MSGINT3,		".msg_int3",	TYPE_INT,			TYPE_MESSAGE,		AI_EvalField_MsgInt3)
DEFINE_FIELD(535,   FIELD_MSGINT4,		".msg_int4",	TYPE_INT,			TYPE_MESSAGE,		AI_EvalField_MsgInt4)
DEFINE_FIELD(536,   FIELD_MSGINT5,		".msg_int5",	TYPE_INT,			TYPE_MESSAGE,		AI_EvalField_MsgInt5)

DEFINE_FIELD(550,   FIELD_DESIGNFLAGS,	".des_flags",	TYPE_INT,			TYPE_GAMEOBJECT,	AI_EvalField_DesignFlags)
DEFINE_FIELD(551,   FIELD_DESIGNINT1,	".des_int1",	TYPE_INT,			TYPE_GAMEOBJECT,	AI_EvalField_DesignInt1)
DEFINE_FIELD(552,   FIELD_DESIGNINT2,	".des_int2",	TYPE_INT,			TYPE_GAMEOBJECT,	AI_EvalField_DesignInt2)
DEFINE_FIELD(553,   FIELD_DESIGNFLOAT1,	".des_float1",	TYPE_FLOAT,			TYPE_GAMEOBJECT,	AI_EvalField_DesignFloat1)
DEFINE_FIELD(554,   FIELD_DESIGNFLOAT2,	".des_float2",	TYPE_FLOAT,			TYPE_GAMEOBJECT,	AI_EvalField_DesignFloat2)
DEFINE_FIELD(555,   FIELD_DESIGNVEC1,	".des_vec1",	TYPE_VECTOR,		TYPE_GAMEOBJECT,	AI_EvalField_DesignVec1)
DEFINE_FIELD(556,   FIELD_DESIGNVEC2,	".des_vec2",	TYPE_VECTOR,		TYPE_GAMEOBJECT,	AI_EvalField_DesignVec2)
DEFINE_FIELD(557,   FIELD_DESIGNGO1,	".des_object1",	TYPE_GAMEOBJECT,	TYPE_GAMEOBJECT,	AI_EvalField_DesignGO1)
DEFINE_FIELD(558,   FIELD_DESIGNGO2,	".des_object2",	TYPE_GAMEOBJECT,	TYPE_GAMEOBJECT,	AI_EvalField_DesignGO2)
DEFINE_FIELD(559,   FIELD_DESIGNNET1,	".des_net1",	TYPE_NETWORK,		TYPE_GAMEOBJECT,	AI_EvalField_DesignNet1)
DEFINE_FIELD(560,   FIELD_DESIGNNET2,	".des_net2",	TYPE_NETWORK,		TYPE_GAMEOBJECT,	AI_EvalField_DesignNet2)
DEFINE_FIELD(561,   FIELD_DESIGNTEXT1,	".des_text1",	TYPE_TEXT,			TYPE_GAMEOBJECT,	AI_EvalField_DesignText1)
DEFINE_FIELD(562,   FIELD_DESIGNTEXT2,	".des_text2",	TYPE_TEXT,			TYPE_GAMEOBJECT,	AI_EvalField_DesignText2)
DEFINE_FIELD(563,   FIELD_DESIGNINT3,	".des_int3",	TYPE_INT,			TYPE_GAMEOBJECT,	AI_EvalField_DesignInt3)
DEFINE_FIELD(564,   FIELD_DESIGNDFLAGS,	".des_desflags",TYPE_INT,			TYPE_GAMEOBJECT,	AI_EvalField_DesignDesFlags)
/*$on*/

#undef DEFINE_FIELD
