/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

internal r32
CalculateBoundingRadius(entity *Entity)
{
    r32 MaxRadius = 0.0f;
    for(collision_shape *Shape = Entity->CollisionShapes;
        Shape;
        Shape = Shape->Next)
    {
        r32 Radius = 0.0f;
        switch(Shape->Type)
        {
            case CollisionShapeType_Circle:
            {
                Radius = Length(Shape->Center) + Shape->Radius;
            } break;

            case CollisionShapeType_Triangle:
            {
                Radius = Maximum(Maximum(Length(Shape->A),
                                         Length(Shape->B)),
                                 Length(Shape->C));
            } break;
        }
        MaxRadius = Maximum(Radius, MaxRadius);
    }
    r32 Result = MaxRadius + 1.0f;
    return(Result);
}

inline collision_shape
CollisionTriangle(v2 PointA, v2 PointB, v2 PointC)
{
    collision_shape Result;
    Result.Type = CollisionShapeType_Triangle;
    Result.A = PointA;
    Result.B = PointB;
    Result.C = PointC;
    return(Result);
}

// TODO(chris): Allow a special shape for triangle strips?
#define CollisionTriangleStrip(Name, Strip)                             \
    collision_shape Name[ArrayCount(Strip)-2];                          \
    for(u32 i=0;i<ArrayCount(Name);++i)                                 \
        Name[i]=(i&1)?CollisionTriangle(Strip[i],Strip[i+2],Strip[i+1]) : \
            CollisionTriangle(Strip[i],Strip[i+1],Strip[i+2]);

#define CombineShapes(Name, Shapes1, Shapes2)                           \
    collision_shape Name[ArrayCount(Shapes1)+ArrayCount(Shapes2)];      \
    for(u32 i=0;i<ArrayCount(Shapes1);++i)Name[i]=Shapes1[i];           \
    for(u32 i=0;i<ArrayCount(Shapes2);++i)Name[ArrayCount(Shapes1)+i]=Shapes2[i];

// TODO(chris): Allow a special shape for rectangles?
#define CollisionRectangle(Rect) \
    CollisionTriangle((Rect).Min, V2((Rect).Max.x, (Rect).Min.y), (Rect).Max), \
    CollisionTriangle((Rect).Max, V2((Rect).Min.x, (Rect).Max.y), (Rect).Min)

inline collision_shape
CollisionCircle(r32 Radius, v2 Center = V2(0, 0))
{
    collision_shape Result;
    Result.Type = CollisionShapeType_Circle;
    Result.Center = Center;
    Result.Radius = Radius;
    return(Result);
}

inline collision_shape
CollisionCircleDiameter(r32 Diameter, v2 Center = V2(0, 0))
{
    collision_shape Result = CollisionCircle(0.5f*Diameter, Center);
}

inline entity *
CreateShip(play_state *State, v3 P, r32 Yaw)
{
    v2 Dim = V2(10.0f, 10.0f);
    v2 HalfDim = Dim*0.5f;
#if 1
    collision_shape Shapes[] =
    {
        CollisionTriangle(0.97f*V2(HalfDim.x, 0),
                          0.96f*V2(-HalfDim.x, HalfDim.y),
                          0.69f*V2(-HalfDim.x, 0)),
        CollisionTriangle(0.97f*V2(HalfDim.x, 0),
                          0.69f*V2(-HalfDim.x, 0),
                          0.96f*V2(-HalfDim.x, -HalfDim.y)),
    };
#else
    collision_shape Shapes[] =
    {
        CollisionCircle(1.0f, V2(10.0f, 0.0f)),
    };
#endif

    entity *Result = CreateEntity(State, ArrayCount(Shapes), Shapes);
    Result->Type = EntityType_Ship;
    Result->ColliderType = ColliderType_Ship;
    Result->Dim = Dim;
    Result->P = P;
    Result->Yaw = Yaw;

    return(Result);
}

inline entity *
CreateFloatingHead(play_state *State)
{
    entity *Result = CreateEntity(State);
    Result->Type = EntityType_FloatingHead;
    Result->ColliderType = ColliderType_None;
    Result->P = V3(0.0f, 0.0f, 0.0f);
    Result->Dim = V2(100.0f, 100.0f);
    Result->RotationMatrix =
    {
        1, 0, 0,
        0, 1, 0,
        0, 0, 1,
    };
    return(Result);
}

inline entity *
CreateAsteroid(play_state *State, v3 P, r32 Radius, v3 dP)
{
#if 1
    collision_shape Shapes[] =
    {
        CollisionCircle(Radius),
    };
#else
    collision_shape Shapes[] =
    {
//        CollisionTriangle(V2(-Radius, -Radius),
//                          V2(Radius, Radius),
//                          V2(-Radius, Radius)),
        CollisionTriangle(V2(-Radius, -Radius),
                          V2(Radius, -Radius),
                          V2(Radius, Radius)),
    };
#endif

    entity *Result = CreateEntity(State, ArrayCount(Shapes), Shapes);
    Result->ColliderType = ColliderType_Asteroid;
    Result->Type = EntityType_Asteroid;
    Result->P = P;
    Result->dP = dP;
    Result->Dim = 2.0f*V2(Radius, Radius);
    
    return(Result);
}

inline entity *
CreateG(play_state *State, loaded_font *Font, v3 P, r32 Scale)
{
    char Character = 'G';
    loaded_bitmap *Glyph = Font->Glyphs + Character;
    v2 Dim = Scale*V2i(Glyph->Width, Glyph->Height);
    collision_shape Rectangles[] =
    {
        CollisionRectangle(MinMax(V2(0.547f*Dim.x, 0.384f*Dim.y),
                                  V2(1.0f*Dim.x, 0.5f*Dim.y))),
    };
    v2 Strip[] =
    {
        V2(1.0f*Dim.x, 0.384f*Dim.y),
        V2(0.858f*Dim.x, 0.384f*Dim.y),
        V2(1.0f*Dim.x, 0.13f*Dim.y),
        V2(0.858f*Dim.x, 0.2f*Dim.y),
        V2(0.8f*Dim.x, 0.03f*Dim.y),
        V2(0.68f*Dim.x, 0.12f*Dim.y),
        V2(0.63f*Dim.x, -0.01f*Dim.y),
        V2(0.54f*Dim.x, 0.105f*Dim.y),
        V2(0.45f*Dim.x, -0.01f*Dim.y),
        V2(0.35f*Dim.x, 0.14f*Dim.y),
        V2(0.26f*Dim.x, 0.05f*Dim.y),
        V2(0.23f*Dim.x, 0.225f*Dim.y),
        V2(0.13f*Dim.x, 0.14f*Dim.y),
        V2(0.16f*Dim.x, 0.35f*Dim.y),
        V2(0.03f*Dim.x, 0.3f*Dim.y),
        V2(0.14f*Dim.x, 0.43f*Dim.y),
        V2(0.0f*Dim.x, 0.44f*Dim.y),
        V2(0.14f*Dim.x, 0.54f*Dim.y),
        V2(0.001f*Dim.x, 0.59f*Dim.y),
        V2(0.205f*Dim.x, 0.72f*Dim.y),
        V2(0.066f*Dim.x, 0.75f*Dim.y),
        V2(0.3f*Dim.x, 0.815f*Dim.y),
        V2(0.15f*Dim.x, 0.86f*Dim.y),
        V2(0.4f*Dim.x, 0.855f*Dim.y),
        V2(0.275f*Dim.x, 0.94f*Dim.y),
        V2(0.55f*Dim.x, 0.875f*Dim.y),
        V2(0.43f*Dim.x, 0.985f*Dim.y),
        V2(0.66f*Dim.x, 0.86f*Dim.y),
        V2(0.65f*Dim.x, 0.985f*Dim.y),
        V2(0.75f*Dim.x, 0.82f*Dim.y),
        V2(0.81f*Dim.x, 0.93f*Dim.y),
        V2(0.825f*Dim.x, 0.74f*Dim.y),
        V2(0.92f*Dim.x, 0.835f*Dim.y),
        V2(0.85f*Dim.x, 0.67f*Dim.y),
        V2(0.985f*Dim.x, 0.7f*Dim.y),
    };
    CollisionTriangleStrip(Triangles, Strip);
    CombineShapes(Shapes, Rectangles, Triangles)
    
    entity *Result = CreateEntity(State, ArrayCount(Shapes), Shapes);
    Result->ColliderType = ColliderType_Asteroid;
    Result->Type = EntityType_Letter;
    Result->P = P;
    Result->Character = Character;
    Result->Dim = Dim;

    return(Result);
}

inline entity *
CreateA(play_state *State, loaded_font *Font, v3 P, r32 Scale)
{
    char Character = 'A';
    loaded_bitmap *Glyph = Font->Glyphs + Character;
    v2 Dim = Scale*V2i(Glyph->Width, Glyph->Height);

#if 0
    collision_shape Shapes[] =
    {
        CollisionTriangle(V2(0.0f*Dim.x, 0.0f*Dim.y),
                          V2(0.58f*Dim.x, Dim.y),
                          V2(0.423f*Dim.x, Dim.y)),
        CollisionTriangle(V2(1.0f*Dim.x, 0.0f*Dim.y),
                          V2(0.58f*Dim.x, Dim.y),
                          V2(0.423f*Dim.x, Dim.y)),
        CollisionTriangle(V2(0.001f*Dim.x, 0.0f*Dim.y),
                          V2(0.16f*Dim.x, 0.0f*Dim.y),
                          V2(0.57f*Dim.x, Dim.y)),
        CollisionTriangle(V2(1.0f*Dim.x, 0.0f*Dim.y),
                          V2(0.457f*Dim.x, Dim.y),
                          V2(0.84f*Dim.x, 0.0f*Dim.y)),
        CollisionRectangle(MinMax(V2(0.2f*Dim.x, 0.304f*Dim.y),
                                  V2(0.8f*Dim.x, 0.425f*Dim.y))),
    };
#endif

    v2 Strip1[] =
    {
        V2(0.0f*Dim.x, 0.0f*Dim.y),
        V2(0.158f*Dim.x, 0.0f*Dim.y),
        V2(0.425f*Dim.x, 1.0f*Dim.y),
        V2(0.51f*Dim.x, 0.87f*Dim.y),
        V2(0.575f*Dim.x, 1.0f*Dim.y),
        V2(0.844f*Dim.x, 0.0f*Dim.y),
        V2(1.0f*Dim.x, 0.0f*Dim.y),
    };
    v2 Strip2[] =
    {
        V2(0.33f*Dim.x, 0.425f*Dim.y),
        V2(0.28f*Dim.x, 0.304f*Dim.y),
        V2(0.682f*Dim.x, 0.425f*Dim.y),
        V2(0.73f*Dim.x, 0.304f*Dim.y),
    };
    CollisionTriangleStrip(Shapes1, Strip1);
    CollisionTriangleStrip(Shapes2, Strip2);
    CombineShapes(Shapes, Shapes1, Shapes2);
    
    entity *Result = CreateEntity(State, ArrayCount(Shapes), Shapes);
    Result->ColliderType = ColliderType_Asteroid;
    Result->Type = EntityType_Letter;
    Result->P = P;
    Result->Character = Character;
    Result->Dim = Dim;

    return(Result);
}

inline entity *
CreateM(play_state *State, loaded_font *Font, v3 P, r32 Scale)
{
    char Character = 'M';
    loaded_bitmap *Glyph = Font->Glyphs + Character;
    v2 Dim = Scale*V2i(Glyph->Width, Glyph->Height);
    v2 Strip[] =
    {
        V2(0.0f*Dim.x, 0.0f*Dim.y),
        V2(0.14f*Dim.x, 0.0f*Dim.y),
        V2(0.0f*Dim.x, 1.0f*Dim.y),
        V2(0.14f*Dim.x, 0.85f*Dim.y),
        V2(0.225f*Dim.x, 1.0f*Dim.y),
        V2(0.415f*Dim.x, 0.0f*Dim.y),
        V2(0.5f*Dim.x, 0.15f*Dim.y),
        V2(0.585f*Dim.x, 0.0f*Dim.y),
        V2(0.775f*Dim.x, 1.0f*Dim.y),
        V2(0.86f*Dim.x, 0.85f*Dim.y),
        V2(1.0f*Dim.x, 1.0f*Dim.y),
        V2(0.86f*Dim.x, 0.0f*Dim.y),
        V2(1.0f*Dim.x, 0.0f*Dim.y),
    };
    CollisionTriangleStrip(Shapes, Strip);

    entity *Result = CreateEntity(State, ArrayCount(Shapes), Shapes);
    Result->ColliderType = ColliderType_Asteroid;
    Result->Type = EntityType_Letter;
    Result->P = P;
    Result->Character = Character;
    Result->Dim = Dim;
    
    return(Result);
}

inline entity *
CreateE(play_state *State, loaded_font *Font, v3 P, r32 Scale)
{
    char Character = 'E';
    loaded_bitmap *Glyph = Font->Glyphs + Character;
    v2 Dim = Scale*V2i(Glyph->Width, Glyph->Height);
    collision_shape Shapes[] =
    {
        CollisionRectangle(MinMax(V2(0.0f*Dim.x, 0.0f*Dim.y),
                                  V2(0.18f*Dim.x, 1.0f*Dim.y))),
        CollisionRectangle(MinMax(V2(0.18f*Dim.x, 0.0f*Dim.y),
                                  V2(1.0f*Dim.x, 0.12f*Dim.y))),
        CollisionRectangle(MinMax(V2(0.18f*Dim.x, 0.88f*Dim.y),
                                  V2(0.965f*Dim.x, 1.0f*Dim.y))),
        CollisionRectangle(MinMax(V2(0.18f*Dim.x, 0.465f*Dim.y),
                                  V2(0.915f*Dim.x, 0.586f*Dim.y))),
    };

    entity *Result = CreateEntity(State, ArrayCount(Shapes), Shapes);
    Result->ColliderType = ColliderType_Asteroid;
    Result->Type = EntityType_Letter;
    Result->P = P;
    Result->Character = Character;
    Result->Dim = Dim;

    return(Result);
}

inline entity *
CreateO(play_state *State, loaded_font *Font, v3 P, r32 Scale)
{
    char Character = 'O';
    loaded_bitmap *Glyph = Font->Glyphs + Character;
    v2 Dim = Scale*V2i(Glyph->Width, Glyph->Height);
    v2 Strip[] =
    {
        V2(0.9f*Dim.x, 0.16f*Dim.y),
        V2(0.75f*Dim.x, 0.2f*Dim.y),
        V2(0.77f*Dim.x, 0.05f*Dim.y),
        V2(0.65f*Dim.x, 0.13f*Dim.y),
        V2(0.58f*Dim.x, -0.01f*Dim.y),
        V2(0.54f*Dim.x, 0.105f*Dim.y),
        V2(0.41f*Dim.x, -0.01f*Dim.y),
        V2(0.33f*Dim.x, 0.14f*Dim.y),
        V2(0.24f*Dim.x, 0.05f*Dim.y),
        V2(0.215f*Dim.x, 0.225f*Dim.y),
        V2(0.12f*Dim.x, 0.14f*Dim.y),
        V2(0.154f*Dim.x, 0.35f*Dim.y),
        V2(0.025f*Dim.x, 0.3f*Dim.y),
        V2(0.14f*Dim.x, 0.43f*Dim.y),
        V2(0.0f*Dim.x, 0.44f*Dim.y),
        V2(0.14f*Dim.x, 0.54f*Dim.y),
        V2(0.001f*Dim.x, 0.59f*Dim.y),
        V2(0.195f*Dim.x, 0.72f*Dim.y),
        V2(0.06f*Dim.x, 0.75f*Dim.y),
        V2(0.3f*Dim.x, 0.815f*Dim.y),
        V2(0.144f*Dim.x, 0.86f*Dim.y),
        V2(0.4f*Dim.x, 0.855f*Dim.y),
        V2(0.27f*Dim.x, 0.94f*Dim.y),
        V2(0.55f*Dim.x, 0.872f*Dim.y),
        V2(0.4f*Dim.x, 0.985f*Dim.y),
        V2(0.61f*Dim.x, 0.86f*Dim.y),
        V2(0.6f*Dim.x, 0.985f*Dim.y),
        V2(0.7f*Dim.x, 0.82f*Dim.y),
        V2(0.76f*Dim.x, 0.93f*Dim.y),
        V2(0.78f*Dim.x, 0.74f*Dim.y),
        V2(0.88f*Dim.x, 0.835f*Dim.y),
        V2(0.84f*Dim.x, 0.62f*Dim.y),
        V2(0.962f*Dim.x, 0.7f*Dim.y),
        V2(0.86f*Dim.x, 0.52f*Dim.y),
        V2(1.0f*Dim.x, 0.57f*Dim.y),
        V2(0.86f*Dim.x, 0.44f*Dim.y),
        V2(0.995f*Dim.x, 0.38f*Dim.y),
        V2(0.84f*Dim.x, 0.34f*Dim.y),
        V2(0.97f*Dim.x, 0.3f*Dim.y),
        V2(0.795f*Dim.x, 0.25f*Dim.y),
        V2(0.935f*Dim.x, 0.22f*Dim.y),
        V2(0.75f*Dim.x, 0.2f*Dim.y),
        V2(0.9f*Dim.x, 0.16f*Dim.y),
    };
    CollisionTriangleStrip(Shapes, Strip);
    
    entity *Result = CreateEntity(State, ArrayCount(Shapes), Shapes);
    Result->ColliderType = ColliderType_Asteroid;
    Result->Type = EntityType_Letter;
    Result->P = P;
    Result->Character = Character;
    Result->Dim = Dim;

    return(Result);
}

inline entity *
CreateV(play_state *State, loaded_font *Font, v3 P, r32 Scale)
{
    char Character = 'V';
    loaded_bitmap *Glyph = Font->Glyphs + Character;
    v2 Dim = Scale*V2i(Glyph->Width, Glyph->Height);
    v2 Strip[] =
    {
        V2(1.0f*Dim.x, 1.0f*Dim.y),
        V2(0.84f*Dim.x, 1.0f*Dim.y),
        V2(0.58f*Dim.x, 0.0f*Dim.y),
        V2(0.5f*Dim.x, 0.12f*Dim.y),
        V2(0.42f*Dim.x, 0.0f*Dim.y),
        V2(0.16f*Dim.x, 1.0f*Dim.y),
        V2(0.0f*Dim.x, 1.0f*Dim.y),
    };
    CollisionTriangleStrip(Shapes, Strip);
    
    entity *Result = CreateEntity(State, ArrayCount(Shapes), Shapes);
    Result->ColliderType = ColliderType_Asteroid;
    Result->Type = EntityType_Letter;
    Result->P = P;
    Result->Character = Character;
    Result->Dim = Dim;
    
    return(Result);
}

inline entity *
CreateR(play_state *State, loaded_font *Font, v3 P, r32 Scale)
{
    char Character = 'R';
    loaded_bitmap *Glyph = Font->Glyphs + Character;
    v2 Dim = Scale*V2i(Glyph->Width, Glyph->Height);
#if 0
    collision_shape Shapes[] =
    {
        CollisionRectangle(MinMax(V2(0.0f*Dim.x, 0.0f*Dim.y),
                                  V2(0.155f*Dim.x, 1.0f*Dim.y))),
        CollisionRectangle(MinMax(V2(0.0f*Dim.x, 0.44f*Dim.y),
                                  V2(0.608f*Dim.x, 0.56f*Dim.y))),
        CollisionRectangle(MinMax(V2(0.0f*Dim.x, 0.88f*Dim.y),
                                  V2(0.62f*Dim.x, 1.0f*Dim.y))),
        CollisionTriangle(V2(0.55f*Dim.x, 0.88f*Dim.y),
                          V2(0.9f*Dim.x, 0.82f*Dim.y),
                          V2(0.62f*Dim.x, 1.0f*Dim.y)),
        CollisionTriangle(V2(0.53f*Dim.x, 0.56f*Dim.y),
                          V2(0.608f*Dim.x, 0.44f*Dim.y),
                          V2(0.9f*Dim.x, 0.62f*Dim.y)),
        CollisionTriangle(V2(0.82f*Dim.x, 0.0f*Dim.y),
                          V2(1.0f*Dim.x, 0.0f*Dim.y),
                          V2(0.39f*Dim.x, 0.56f*Dim.y)),
        CollisionTriangle(V2(0.33f*Dim.x, 0.45f*Dim.y),
                          V2(0.62f*Dim.x, 0.39f*Dim.y),
                          V2(0.62f*Dim.x, 0.45f*Dim.y)
                          ),
        CollisionTriangle(V2(0.43f*Dim.x, 0.43f*Dim.y),
                          V2(0.6f*Dim.x, 0.34f*Dim.y),
                          V2(0.6f*Dim.x, 0.4f*Dim.y)
                          ),
        CollisionTriangle(V2(1.0f*Dim.x, 0.0f*Dim.y),
                          V2(0.785f*Dim.x, 0.3f*Dim.y),
                          V2(0.685f*Dim.x, 0.4f*Dim.y)
                          ),
        CollisionTriangle(V2(0.35f*Dim.x, 0.55f*Dim.y),
                          V2(1.0f*Dim.x, 0.0f*Dim.y),
                          V2(0.77f*Dim.x, 0.3f*Dim.y)
                          ),
        CollisionTriangle(V2(0.97f*Dim.x, 0.05f*Dim.y),
                          V2(0.685f*Dim.x, 0.4f*Dim.y),
                          V2(0.45f*Dim.x, 0.55f*Dim.y)
                          ),
        CollisionTriangle(V2(0.63f*Dim.x, 0.55f*Dim.y),
                          V2(0.912f*Dim.x, 0.65f*Dim.y),
                          V2(0.912f*Dim.x, 0.8f*Dim.y)
                          ),
        CollisionTriangle(V2(0.8f*Dim.x, 0.68f*Dim.y),
                          V2(0.75f*Dim.x, 0.97f*Dim.y),
                          V2(0.62f*Dim.x, 1.0f*Dim.y)
                          ),
        CollisionTriangle(V2(0.8f*Dim.x, 0.68f*Dim.y),
                          V2(0.84f*Dim.x, 0.915f*Dim.y),
                          V2(0.75f*Dim.x, 0.97f*Dim.y)
                          ),
        CollisionTriangle(V2(0.8f*Dim.x, 0.68f*Dim.y),
                          V2(0.912f*Dim.x, 0.8f*Dim.y),
                          V2(0.84f*Dim.x, 0.915f*Dim.y)
                          ),
        CollisionTriangle(V2(0.73f*Dim.x, 0.62f*Dim.y),
                          V2(0.85f*Dim.x, 0.55f*Dim.y),
                          V2(0.912f*Dim.x, 0.65f*Dim.y)
                          ),
        CollisionTriangle(V2(0.71f*Dim.x, 0.5f*Dim.y),
                          V2(0.9f*Dim.x, 0.7f*Dim.y),
                          V2(0.8f*Dim.x, 0.9f*Dim.y)
                          ),
        CollisionTriangle(V2(0.6f*Dim.x, 0.9f*Dim.y),
                          V2(0.75f*Dim.x, 0.8f*Dim.y),
                          V2(0.8f*Dim.x, 0.95f*Dim.y)
                          ),
        CollisionTriangle(V2(0.6f*Dim.x, 0.44f*Dim.y),
                          V2(0.78f*Dim.x, 0.5f*Dim.y),
                          V2(0.9f*Dim.x, 0.63f*Dim.y)
                          ),
        CollisionTriangle(V2(0.85f*Dim.x, 0.55f*Dim.y),
                          V2(0.8f*Dim.x, 0.63f*Dim.y),
                          V2(0.78f*Dim.x, 0.5f*Dim.y)
                          ),
    };
#endif
    collision_shape Shapes0[] =
    {
        CollisionRectangle(MinMax(V2(0.0f*Dim.x, 0.0f*Dim.y),
                                  V2(0.155f*Dim.x, 1.0f*Dim.y)))
    };
    v2 Strip1[] =
    {
        V2(0.155f*Dim.x, 1.0f*Dim.y),
        V2(0.155f*Dim.x, 0.88f*Dim.y),
        V2(0.66f*Dim.x, 1.0f*Dim.y),
        V2(0.55f*Dim.x, 0.88f*Dim.y),
        V2(0.77f*Dim.x, 0.965f*Dim.y),
        V2(0.68f*Dim.x, 0.85f*Dim.y),
        V2(0.87f*Dim.x, 0.88f*Dim.y),
        V2(0.75f*Dim.x, 0.78f*Dim.y),
        V2(0.913f*Dim.x, 0.78f*Dim.y),
        V2(0.76f*Dim.x, 0.7f*Dim.y),
        V2(0.913f*Dim.x, 0.66f*Dim.y),
        V2(0.7f*Dim.x, 0.6f*Dim.y),
        V2(0.845f*Dim.x, 0.55f*Dim.y),
        V2(0.54f*Dim.x, 0.56f*Dim.y),
        V2(0.73f*Dim.x, 0.48f*Dim.y),
        V2(0.38f*Dim.x, 0.44f*Dim.y),
        V2(0.6f*Dim.x, 0.44f*Dim.y),
        V2(0.49f*Dim.x, 0.41f*Dim.y),
        V2(0.66f*Dim.x, 0.415f*Dim.y),
        V2(0.53f*Dim.x, 0.38f*Dim.y),
        V2(0.75f*Dim.x, 0.34f*Dim.y),
        V2(0.82f*Dim.x, 0.0f*Dim.y),
        V2(1.0f*Dim.x, 0.0f*Dim.y),
    };
    v2 Strip2[] =
    {
        V2(0.155f*Dim.x, 0.56f*Dim.y),
        V2(0.155f*Dim.x, 0.44f*Dim.y),
        V2(0.54f*Dim.x, 0.56f*Dim.y),
        V2(0.38f*Dim.x, 0.44f*Dim.y),
    };
    CollisionTriangleStrip(Shapes1, Strip1);
    CollisionTriangleStrip(Shapes2, Strip2);
    CombineShapes(Combo, Shapes0, Shapes1);
    CombineShapes(Shapes, Combo, Shapes2);

    entity *Result = CreateEntity(State, ArrayCount(Shapes), Shapes);
    Result->ColliderType = ColliderType_Asteroid;
    Result->Type = EntityType_Letter;
    Result->P = P;
    Result->Character = Character;
    Result->Dim = Dim;

    return(Result);
}

inline entity *
CreateBullet(play_state *State, v3 P, v3 dP, r32 Yaw)
{
    v2 Dim = V2(1.5f, 3.0f);
    collision_shape Shapes[] =
    {
        CollisionRectangle(CenterDim(V2(0, 0), V2(0.75f*Dim.y, 0.3f*Dim.x))),
    };
    
    entity *Result = CreateEntity(State, ArrayCount(Shapes), Shapes);
    Result->ColliderType = ColliderType_Ship;
    Result->Type = EntityType_Bullet;
    Result->P = P;
    Result->dP = dP;
    Result->Yaw = Yaw;
    Result->Dim = Dim;
    Result->Timer = 2.0f;
    
    return(Result);
}

inline void
GameOver(play_state *State, render_buffer *RenderBuffer, loaded_font *Font)
{
    v2 ScreenCenter = 0.5f*V2i(RenderBuffer->Width, RenderBuffer->Height); 
    text_layout Layout = {};
    Layout.Font = Font;
    Layout.Scale = 1.0f;
    Layout.Color = V4(1, 1, 1, 1);
    char GameOverText[] = "GAME OVER";
    text_measurement GameOverMeasurement = DrawText(0, &Layout,
                                                    sizeof(GameOverText)-1, GameOverText,
                                                    DrawTextFlags_Measure);
    Layout.P = ScreenCenter + GetTightCenteredOffset(GameOverMeasurement);

    v3 P = Unproject(RenderBuffer, Layout.P, 0.0f);

    r32 GameOverWidth = GameOverMeasurement.MaxX - GameOverMeasurement.MinX;
    v3 EndP = Unproject(RenderBuffer, Layout.P + V2(GameOverWidth, 0), 0.0f);
    r32 Scale = (EndP-P).x / GameOverWidth;

    CreateG(State, Font, P, Scale);
    P.x += Scale*GetTextAdvance(Font, 'G', 'A');
    CreateA(State, Font, P, Scale);
    P.x += Scale*GetTextAdvance(Font, 'A', 'M');
    CreateM(State, Font, P, Scale);
    P.x += Scale*GetTextAdvance(Font, 'M', 'E');
    CreateE(State, Font, P, Scale);
    P.x += Scale*(GetTextAdvance(Font, 'E', ' ') + GetTextAdvance(Font, ' ', 'O'));
    CreateO(State, Font, P, Scale);
    P.x += Scale*GetTextAdvance(Font, 'O', 'V');
    CreateV(State, Font, P, Scale);
    P.x += Scale*GetTextAdvance(Font, 'V', 'E');
    CreateE(State, Font, P, Scale);
    P.x += Scale*GetTextAdvance(Font, 'E', 'R');
    CreateR(State, Font, P, Scale);
}

internal void
ResetGame(play_state *State, render_buffer *RenderBuffer, loaded_font *Font)
{
    RenderBuffer->CameraP = V3(State->ShipStartingP.xy, 100.0f);
    RenderBuffer->CameraRot = 0.0f;

    // NOTE(chris): Null entity
    State->EntityCount = 1;
    State->PhysicsState.ShapeArena.Used = 0;
    State->PhysicsState.FirstFreeShape = 0;

    State->Lives = 3;

    State->ShipStartingP = V3(0, 0, 0);
    State->ShipStartingYaw = 0.0f;
    if(State->Lives)
    {
        entity *Ship = CreateShip(State, State->ShipStartingP, State->ShipStartingYaw);
    }
    else
    {
        GameOver(State, RenderBuffer, Font);
    }
    //entity *FloatingHead = CreateFloatingHead(State);

    entity *SmallAsteroid = CreateAsteroid(State, V3(50.0f, 0.0f, 0.0f), 5.0f, V3(2.0f, 3.0f, 0));
#if 0
    SmallAsteroid->P.x = SmallAsteroid->CollisionShapes[0].Radius +
        Ship->CollisionShapes[0].A.x;
#endif
    entity *SmallAsteroid3 = CreateAsteroid(State, State->ShipStartingP + V3(0, 40.0f, 0), 5.0f, V3(-2.0f, 3.0f, 0));
    entity *SmallAsteroid2 = CreateAsteroid(State, State->ShipStartingP + V3(0, 20.0f, 0), 5.0f, V3(2.0f, -1.0f, 0));
    entity *LargeAsteroid = CreateAsteroid(State, SmallAsteroid->P + V3(30.0f, 30.0f, 0.0f), 10.0f, V3(-0.5f, -1.3f, 0));
}

internal void
PlayMode(game_memory *GameMemory, game_input *Input, loaded_bitmap *BackBuffer)
{
    game_state *GameState = (game_state *)GameMemory->PermanentMemory;
    transient_state *TranState = (transient_state *)GameMemory->TemporaryMemory;
    play_state *State = &GameState->PlayState;

    render_buffer *RenderBuffer = &TranState->RenderBuffer;
    
    if(!State->IsInitialized)
    {
        State->IsInitialized = true;

        InitializePhysicsState(&State->PhysicsState, &GameState->Arena);

        AllowCollision(&State->PhysicsState, ColliderType_Ship, ColliderType_Asteroid);
        AllowCollision(&State->PhysicsState, ColliderType_Asteroid, ColliderType_Asteroid);

        ResetGame(State, RenderBuffer, &GameMemory->Font);
    }
    
    {DEBUG_GROUP("Play Mode");
        DEBUG_FILL_BAR("Entities", State->EntityCount, ArrayCount(State->Entities));
        DEBUG_VALUE("Shape Arena", State->PhysicsState.ShapeArena);
        DEBUG_VALUE("Shape Freelist", (b32)State->PhysicsState.FirstFreeShape);
    }
    
    temporary_memory RenderMemory = BeginTemporaryMemory(&RenderBuffer->Arena);
    PushClear(RenderBuffer, V4(0.0f, 0.0f, 0.0f, 1.0f));
    v2 ScreenCenter = 0.5f*V2i(RenderBuffer->Width, RenderBuffer->Height); 

    game_controller *Keyboard = &Input->Keyboard;
    game_controller *ShipController = Input->GamePads + 0;
    game_controller *AsteroidController = Input->GamePads + 1;

    b32 FirstAsteroid = true;
    // NOTE(chris): Pre collision pass
    for(u32 EntityIndex = 1;
        EntityIndex < State->EntityCount;
        )
    {
        u32 NextIndex = EntityIndex + 1;
        entity *Entity = State->Entities + EntityIndex;
        v3 Facing = V3(Cos(Entity->Yaw), Sin(Entity->Yaw), 0.0f);
        switch(Entity->Type)
        {
            case EntityType_Ship:
            {
                if(Keyboard->Start.EndedDown || ShipController->Start.EndedDown)
                {
                    Entity->P = State->ShipStartingP;
                }

                r32 LeftStickX = (Keyboard->LeftStick.x ?
                                  Keyboard->LeftStick.x :
                                  ShipController->LeftStick.x);
                r32 Thrust;
                if(Keyboard->LeftStick.y)
                {
                    Thrust = Clamp01(Keyboard->LeftStick.y);
                }
                else
                {
                    Thrust = Clamp01(ShipController->LeftStick.y);
                    ShipController->LowFrequencyMotor = Thrust;
                }

#if COLLISION_FINE_DEBUG
                r32 ddYaw = -100.0f*LeftStickX;
#else
                r32 ddYaw = -1.0f*LeftStickX;
#endif
                r32 dYaw = ddYaw*Input->dtForFrame;
                r32 MaxdYawPerFrame = 0.49f*Tau;
                r32 MaxdYawPerSecond = MaxdYawPerFrame/Input->dtForFrame;
                Entity->dYaw = Clamp(-MaxdYawPerSecond, Entity->dYaw + dYaw, MaxdYawPerSecond);
    
                v3 Acceleration = {};
                if((WentDown(ShipController->RightShoulder1) || WentDown(Keyboard->RightShoulder1)))
                {
                    if(Entity->Timer <= 0.0f)
                    {
                        entity *Bullet = CreateBullet(State, Entity->P, Entity->dP + Facing*100.0f,
                                                      Entity->Yaw);
                        r32 BulletOffset = 0.5f*(Entity->Dim.y + Bullet->Dim.y);
                        Bullet->P += Facing*(BulletOffset);
                        Entity->Timer = 0.1f;
                        Acceleration += -Facing*100.0f;
                    }
                }

#if COLLISION_FINE_DEBUG
                Acceleration += 5000.0f*Facing*Thrust;
#else
                Acceleration += 50.0f*Facing*Thrust;
#endif
                Entity->dP += Acceleration*Input->dtForFrame;

                if(Entity->Timer > 0.0f)
                {
                    Entity->Timer -= Input->dtForFrame;
                    ShipController->HighFrequencyMotor = 1.0f;
                }
                else
                {
                    ShipController->HighFrequencyMotor = 0.0f;
                }

            } break;

            case EntityType_Asteroid:
            {
                if(FirstAsteroid)
                {
                    FirstAsteroid = false;
    
                    Entity->dP += 100.0f*V3(AsteroidController->LeftStick, 0)*Input->dtForFrame;
                }

                // TODO(chris): Destroy if out of bounds
            } break;

            case EntityType_Bullet:
            {
                if(Entity->Timer <= 0.0f)
                {
                    DestroyEntity(State, Entity);
                    NextIndex = EntityIndex;
                }
                else
                {
                    Entity->Timer -= Input->dtForFrame;
                }
            } break;

            case EntityType_FloatingHead:
            {
#if 0
                if(Keyboard->ActionUp.EndedDown || ShipController->ActionUp.EndedDown)
                {
                    Entity->P.z += 100.0f*Input->dtForFrame;
                }
                if(Keyboard->ActionDown.EndedDown || ShipController->ActionDown.EndedDown)
                {
                    Entity->P.z -= 100.0f*Input->dtForFrame;
                }
#endif
                r32 YRotation = Input->dtForFrame*(Keyboard->RightStick.x ?
                                                   Keyboard->RightStick.x :
                                                   ShipController->RightStick.x);
                r32 XRotation = Input->dtForFrame*(Keyboard->RightStick.y ?
                                                   Keyboard->RightStick.y :
                                                   ShipController->RightStick.y);
                r32 ZRotation = Input->dtForFrame*((Keyboard->LeftTrigger ?
                                                    Keyboard->LeftTrigger :
                                                    ShipController->LeftTrigger) -
                                                   (Keyboard->RightTrigger ?
                                                    Keyboard->RightTrigger :
                                                    ShipController->RightTrigger));

                r32 CosXRotation = Cos(XRotation);
                r32 SinXRotation = Sin(XRotation);
                r32 CosYRotation = Cos(YRotation);
                r32 SinYRotation = Sin(YRotation);
                r32 CosZRotation = Cos(ZRotation);
                r32 SinZRotation = Sin(ZRotation);

                m33 XRotationMatrix =
                    {
                        1, 0,             0,
                        0, CosXRotation, -SinXRotation,
                        0, SinXRotation,  CosXRotation,
                    };
                m33 YRotationMatrix =
                    {
                        CosYRotation, 0, SinYRotation,
                        0,            1, 0,
                        -SinYRotation, 0, CosYRotation,
                    };
                m33 ZRotationMatrix =
                    {
                        CosZRotation, -SinZRotation, 0,
                        SinZRotation,  CosZRotation, 0,
                        0,             0,            1,
                    };

                Entity->RotationMatrix =
                    ZRotationMatrix*YRotationMatrix*XRotationMatrix*Entity->RotationMatrix;
            } break;

            default:
            {
            } break;
        }
        if(CanCollide(Entity))
        {
            Entity->BoundingRadius = CalculateBoundingRadius(Entity);
#if COLLISION_DEBUG

            Entity->UsedLinearIterations = 0;
            Entity->UsedAngularIterations = 0;

            Entity->CollisionStepP[0] = Entity->P;
            Entity->LinearBoundingCircleCollided[0] = false;
            Entity->LinearCollidingShapeMask[0] = 0;
            Entity->CollisionStepYaw[0] = Entity->Yaw;
            Entity->AngularBoundingCircleCollided[0] = false;
            Entity->AngularCollidingShapeMask[0] = 0;
#endif
        }
        EntityIndex = NextIndex;
    }

    BEGIN_TIMED_BLOCK("Collision");
    // NOTE(chris): Collision pass
    for(u32 EntityIndex = 1;
        EntityIndex < State->EntityCount;
        ++EntityIndex)
    {
        entity *Entity = State->Entities + EntityIndex;
#if COLLISION_FINE_DEBUG
        v3 InitP = Entity->P;
        v3 InitdP = Entity->dP;
        r32 InitYaw = Entity->Yaw;
        r32 InitdYaw = Entity->dYaw;
#endif
        r32 tMax = 1.0f;
        if(CanCollide(Entity))
        {
            for(u32 CollisionIndex = 1;
                CollisionIndex <= COLLISION_ITERATIONS;
                ++CollisionIndex)
            {
                v3 OldP = Entity->P;
                v3 NewP = OldP + Input->dtForFrame*Entity->dP;
                v3 dP = NewP - OldP;
                
                r32 OldYaw = Entity->Yaw;
                r32 NewYaw = OldYaw + Input->dtForFrame*Entity->dYaw;
                r32 dYaw = NewYaw - OldYaw;

                if((tMax <= 0.0f) || (dP.x == 0 && dP.y == 0 && dYaw == 0)) break;
       
                entity *CollidedWith = 0;
                collision Collision = {};
                r32 tMove = tMax;
                if(dP.x != 0 || dP.y != 0)
                {
#if COLLISION_DEBUG
                    ++Entity->UsedLinearIterations;
                    Entity->LinearBoundingCircleCollided[Entity->UsedLinearIterations] = false;
                    Entity->LinearCollidingShapeMask[Entity->UsedLinearIterations] = 0;
                    u32 CollidingShapeMask = 0;
                    u32 OtherCollidingShapeMask = 0;
#endif
                    r32 InvdPY = 1.0f / dP.y;
                    for(u32 OtherEntityIndex = 1;
                        OtherEntityIndex < State->EntityCount;
                        ++OtherEntityIndex)
                    {
                        entity *OtherEntity = State->Entities + OtherEntityIndex;
                        if(!CanCollide(State, Entity, OtherEntity) ||
                           (OtherEntityIndex == EntityIndex)) continue;

                        if(BoundingCirclesIntersect(Entity, OtherEntity, NewP))
                        {
#if COLLISION_DEBUG
                            Entity->LinearBoundingCircleCollided[Entity->UsedLinearIterations] = true;
                            OtherEntity->LinearBoundingCircleCollided[OtherEntity->UsedLinearIterations] = true;
                            u32 CollidingShapeMaskIndex = 1;
                            u32 OtherCollidingShapeMaskIndex = 1;
#endif
                            for(collision_shape *Shape = Entity->CollisionShapes;
                                Shape;
                                Shape = Shape->Next)
                            {
                                for(collision_shape *OtherShape = OtherEntity->CollisionShapes;
                                    OtherShape;
                                    OtherShape = OtherShape->Next)
                                {
                                    switch(Shape->Type|OtherShape->Type)
                                    {
                                        case CollisionShapePair_TriangleTriangle:
                                        {
                                            v2 Offset = OtherEntity->P.xy;
                                            
                                            v2 OtherRotatedA = RotateZ(OtherShape->A, OtherEntity->Yaw);
                                            v2 OtherRotatedB = RotateZ(OtherShape->B, OtherEntity->Yaw);
                                            v2 OtherRotatedC = RotateZ(OtherShape->C, OtherEntity->Yaw);
                                            v2 OtherVectors[3] =
                                            {
                                                OtherRotatedB - OtherRotatedA,
                                                OtherRotatedC - OtherRotatedB,
                                                OtherRotatedA - OtherRotatedC,
                                            };
                                            r32 OtherAngles[4] =
                                            {
                                                InverseTan(OtherVectors[0]),
                                                InverseTan(OtherVectors[1]),
                                                InverseTan(OtherVectors[2]),
                                            };
                                            if(OtherAngles[1] < OtherAngles[0])
                                            {
                                                OtherAngles[3] = OtherAngles[0];
                                                OtherAngles[0] = OtherAngles[1];
                                                OtherAngles[1] = OtherAngles[2];
                                                OtherAngles[2] = OtherAngles[3];
                                                
                                                v2 TempVector = OtherVectors[0];
                                                OtherVectors[0] = OtherVectors[1];
                                                OtherVectors[1] = OtherVectors[2];
                                                OtherVectors[2] = TempVector;

                                                Offset += OtherRotatedB;
                                            }
                                            else if(OtherAngles[2] < OtherAngles[1])
                                            {
                                                OtherAngles[3] = OtherAngles[2];
                                                OtherAngles[2] = OtherAngles[1];
                                                OtherAngles[1] = OtherAngles[0];
                                                OtherAngles[0] = OtherAngles[3];
                                                
                                                v2 TempVector = OtherVectors[2];
                                                OtherVectors[2] = OtherVectors[1];
                                                OtherVectors[1] = OtherVectors[0];
                                                OtherVectors[0] = TempVector;

                                                Offset += OtherRotatedC;
                                            }
                                            else
                                            {
                                                Offset += OtherRotatedA;
                                            }

                                            v2 RotatedA = RotateZ(Shape->A, Entity->Yaw);
                                            v2 RotatedB = RotateZ(Shape->B, Entity->Yaw);
                                            v2 RotatedC = RotateZ(Shape->C, Entity->Yaw);
                                            v2 Vectors[3] =
                                            {
                                                -RotatedB + RotatedA,
                                                -RotatedC + RotatedB,
                                                -RotatedA + RotatedC,
                                            };
                                            r32 MinAngleIndex = 0;
                                            r32 Angles[4] =
                                            {
                                                InverseTan(Vectors[0]),
                                                InverseTan(Vectors[1]),
                                                InverseTan(Vectors[2]),
                                            };
                                            if(Angles[1] < Angles[0])
                                            {
                                                Angles[3] = Angles[0];
                                                Angles[0] = Angles[1];
                                                Angles[1] = Angles[2];
                                                Angles[2] = Angles[3];
                                                
                                                v2 TempVector = Vectors[0];
                                                Vectors[0] = Vectors[1];
                                                Vectors[1] = Vectors[2];
                                                Vectors[2] = TempVector;
                                                
                                                Offset -= RotatedB;
                                            }
                                            else if(Angles[2] < Angles[1])
                                            {
                                                Angles[3] = Angles[2];
                                                Angles[2] = Angles[1];
                                                Angles[1] = Angles[0];
                                                Angles[0] = Angles[3];
                                                
                                                v2 TempVector = Vectors[2];
                                                Vectors[2] = Vectors[1];
                                                Vectors[1] = Vectors[0];
                                                Vectors[0] = TempVector;

                                                Offset -= RotatedC;
                                            }
                                            else
                                            {
                                                Offset -= RotatedA;
                                            }

                                            OtherAngles[3] = Angles[3] = REAL32_MAX;

                                            v2 HullPoints[6];
                                            u32 VectorIndex = 0;
                                            u32 OtherVectorIndex = 0;
                                            for(u32 HullPointIndex = 0;
                                                HullPointIndex < 6;
                                                ++HullPointIndex)
                                            {
                                                HullPoints[HullPointIndex] = Offset;
                                                if(OtherAngles[OtherVectorIndex] < Angles[VectorIndex])
                                                {
                                                    Assert(OtherVectorIndex < 3);
                                                    Offset += OtherVectors[OtherVectorIndex++];
                                                }
                                                else 
                                                {
                                                    Assert(VectorIndex < 3);
                                                    Offset += Vectors[VectorIndex++];
                                                }
                                            }

                                            v2 Start = OldP.xy;
                                            v2 End = NewP.xy;
                                            v2 dMove = End-Start;

                                            r32 Intersection01 = PolygonEdgeRayIntersection(
                                                HullPoints[0], HullPoints[1],
                                                Start, End, HullPoints[3]);
                                            r32 Intersection12 = PolygonEdgeRayIntersection(
                                                HullPoints[1], HullPoints[2],
                                                Start, End, HullPoints[4]);
                                            r32 Intersection23 = PolygonEdgeRayIntersection(
                                                HullPoints[2], HullPoints[3],
                                                Start, End, HullPoints[5]);
                                            r32 Intersection34 = PolygonEdgeRayIntersection(
                                                HullPoints[3], HullPoints[4],
                                                Start, End, HullPoints[0]);
                                            r32 Intersection45 = PolygonEdgeRayIntersection(
                                                HullPoints[4], HullPoints[5],
                                                Start, End, HullPoints[1]);
                                            r32 Intersection50 = PolygonEdgeRayIntersection(
                                                HullPoints[5], HullPoints[0],
                                                Start, End, HullPoints[2]);
                                            
                                            b32 Intersection01Updated = ProcessIntersection(Intersection01, &tMove);
                                            b32 Intersection12Updated = ProcessIntersection(Intersection12, &tMove);
                                            b32 Intersection23Updated = ProcessIntersection(Intersection23, &tMove);
                                            b32 Intersection34Updated = ProcessIntersection(Intersection34, &tMove);
                                            b32 Intersection45Updated = ProcessIntersection(Intersection45, &tMove);
                                            b32 Intersection50Updated = ProcessIntersection(Intersection50, &tMove);

                                            b32 Updated = (Intersection01Updated || Intersection12Updated ||
                                                           Intersection23Updated || Intersection34Updated ||
                                                           Intersection45Updated || Intersection50Updated);

                                            if(Updated)
                                            {
                                                Collision.Type = CollisionType_Line;
                                                if(Intersection01Updated)
                                                {
                                                    Collision.A = HullPoints[0];
                                                    Collision.B = HullPoints[1];
                                                }
                                                if(Intersection12Updated)
                                                {
                                                    Collision.A = HullPoints[1];
                                                    Collision.B = HullPoints[2];
                                                }
                                                if(Intersection23Updated)
                                                {
                                                    Collision.A = HullPoints[2];
                                                    Collision.B = HullPoints[3];
                                                }
                                                if(Intersection34Updated)
                                                {
                                                    Collision.A = HullPoints[3];
                                                    Collision.B = HullPoints[4];
                                                }
                                                if(Intersection45Updated)
                                                {
                                                    Collision.A = HullPoints[4];
                                                    Collision.B = HullPoints[5];
                                                }
                                                if(Intersection50Updated)
                                                {
                                                    Collision.A = HullPoints[5];
                                                    Collision.B = HullPoints[0];
                                                }

                                                CollidedWith = OtherEntity;
#if COLLISION_DEBUG
                                                CollidingShapeMask = CollidingShapeMaskIndex;
                                                OtherCollidingShapeMask = OtherCollidingShapeMaskIndex;
#endif
                                            }
                                        } break;

                                        case CollisionShapePair_CircleCircle:
                                        {
                                            v2 Start = Entity->P.xy + RotateZ(Shape->Center, Entity->Yaw);
                                            v2 End = Start + dP.xy;
                                            v2 dMove = End - Start;
                                            r32 HitRadius = Shape->Radius + OtherShape->Radius;
                                            v2 HitCenter = (OtherEntity->P.xy +
                                                            RotateZ(OtherShape->Center, OtherEntity->Yaw));

                                            circle_ray_intersection_result Intersection =
                                                                             CircleRayIntersection(HitCenter, HitRadius, Start, End);

                                            if(ProcessIntersection(Intersection, &tMove))
                                            {
                                                Collision.Type = CollisionType_Circle;
                                                Collision.Deflection = Start - HitCenter;

                                                CollidedWith = OtherEntity;
#if COLLISION_DEBUG
                                                CollidingShapeMask = CollidingShapeMaskIndex;
                                                OtherCollidingShapeMask = OtherCollidingShapeMaskIndex;
#endif
                                            }
                                        } break;

                                        case CollisionShapePair_TriangleCircle:
                                        {
                                            v2 Start;
                                            v2 End;
                                            r32 Radius;
                                            v2 A, B, C;
                        
                                            if(Shape->Type == CollisionShapeType_Triangle)
                                            {
                                                Start = OtherEntity->P.xy + RotateZ(OtherShape->Center, OtherEntity->Yaw);
                                                End = Start - dP.xy;
                                                Radius = OtherShape->Radius;
                                                A = Entity->P.xy + RotateZ(Shape->A, Entity->Yaw);
                                                B = Entity->P.xy + RotateZ(Shape->B, Entity->Yaw);
                                                C = Entity->P.xy + RotateZ(Shape->C, Entity->Yaw);
                                            }
                                            else
                                            {
                                                Start = Entity->P.xy + RotateZ(Shape->Center, Entity->Yaw);
                                                End = Start + dP.xy;
                                                Radius = Shape->Radius;
                                                A = OtherEntity->P.xy +
                                                    RotateZ(OtherShape->A, OtherEntity->Yaw);
                                                B = OtherEntity->P.xy +
                                                    RotateZ(OtherShape->B, OtherEntity->Yaw);
                                                C = OtherEntity->P.xy +
                                                    RotateZ(OtherShape->C, OtherEntity->Yaw);
                                            }

                                            v2 dMove = End - Start;

                                            v2 AB = B - A;
                                            v2 BC = C - B;
                                            v2 CA = A - C;
                                            r32 ABLength = Length(AB);
                                            r32 BCLength = Length(BC);
                                            r32 CALength = Length(CA);
                                            r32 InvABLength = 1.0f/ABLength;
                                            r32 InvBCLength = 1.0f/BCLength;
                                            r32 InvCALength = 1.0f/CALength;
                                            v2 ABTranslate = -Perp(AB)*InvABLength*Radius;
                                            v2 BCTranslate = -Perp(BC)*InvBCLength*Radius;
                                            v2 CATranslate = -Perp(CA)*InvCALength*Radius;
                                            v2 ABHitA = A + ABTranslate;
                                            v2 ABHitB = B + ABTranslate;
                                            v2 BCHitB = B + BCTranslate;
                                            v2 BCHitC = C + BCTranslate;
                                            v2 CAHitC = C + CATranslate;
                                            v2 CAHitA = A + CATranslate;

                                            circle_ray_intersection_result IntersectionA =
                                                CircleRayIntersection(A, Radius, Start, End);
                                            circle_ray_intersection_result IntersectionB =
                                                CircleRayIntersection(B, Radius, Start, End);
                                            circle_ray_intersection_result IntersectionC =
                                                CircleRayIntersection(C, Radius, Start, End);

                                            r32 IntersectionAB = PolygonEdgeRayIntersection(ABHitA, ABHitB, Start, End, C);
                                            r32 IntersectionBC = PolygonEdgeRayIntersection(BCHitB, BCHitC, Start, End, A);
                                            r32 IntersectionCA = PolygonEdgeRayIntersection(CAHitC, CAHitA, Start, End, B);

                                            b32 IntersectionAUpdated = ProcessIntersection(IntersectionA, &tMove);
                                            b32 IntersectionBUpdated = ProcessIntersection(IntersectionB, &tMove);
                                            b32 IntersectionCUpdated = ProcessIntersection(IntersectionC, &tMove);
                                            b32 IntersectionABUpdated = ProcessIntersection(IntersectionAB, &tMove);
                                            b32 IntersectionBCUpdated = ProcessIntersection(IntersectionBC, &tMove);
                                            b32 IntersectionCAUpdated = ProcessIntersection(IntersectionCA, &tMove);

                                            b32 Updated = (IntersectionAUpdated || IntersectionBUpdated ||
                                                           IntersectionCUpdated || IntersectionABUpdated ||
                                                           IntersectionBCUpdated || IntersectionCAUpdated);

                                            if(Updated)
                                            {
                                                if(OtherShape->Type == CollisionShapeType_Circle)
                                                {
                                                    if(IntersectionAUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Circle;
                                                        Collision.Deflection = A - Start;
                                                    }
                                                    if(IntersectionBUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Circle;
                                                        Collision.Deflection = B - Start;
                                                    }
                                                    if(IntersectionCUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Circle;
                                                        Collision.Deflection = C - Start;
                                                    }
                                                    if(IntersectionABUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Line;
                                                        Collision.A = B;
                                                        Collision.B = A;
                                                    }
                                                    if(IntersectionBCUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Line;
                                                        Collision.A = C;
                                                        Collision.B = B;
                                                    }
                                                    if(IntersectionCAUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Line;
                                                        Collision.A = A;
                                                        Collision.B = C;
                                                    }
                                                }
                                                else
                                                {
                                                    if(IntersectionAUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Circle;
                                                        Collision.Deflection = Start - A;
                                                    }
                                                    if(IntersectionBUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Circle;
                                                        Collision.Deflection = Start - B;
                                                    }
                                                    if(IntersectionCUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Circle;
                                                        Collision.Deflection = Start - C;
                                                    }
                                                    if(IntersectionABUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Line;
                                                        Collision.A = A;
                                                        Collision.B = B;
                                                    }
                                                    if(IntersectionBCUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Line;
                                                        Collision.A = B;
                                                        Collision.B = C;
                                                    }
                                                    if(IntersectionCAUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Line;
                                                        Collision.A = C;
                                                        Collision.B = A;
                                                    }
                                                }

                                                CollidedWith = OtherEntity;
#if COLLISION_DEBUG
                                                CollidingShapeMask = CollidingShapeMaskIndex;
                                                OtherCollidingShapeMask = OtherCollidingShapeMaskIndex;
#endif
                                            }
                                        } break;

                                        default:
                                        {
                                            NotImplemented;
                                        } break;
                                    }
#if COLLISION_DEBUG
                                    OtherCollidingShapeMaskIndex = (OtherCollidingShapeMaskIndex << 1);
#endif

                                }
#if COLLISION_DEBUG
                                CollidingShapeMaskIndex = (CollidingShapeMaskIndex << 1);
#endif
                            }
                        }
                    }
                    Entity->P += dP*tMove;
                    
                    if(CollidedWith)
                    {
#if COLLISION_DEBUG
                        Entity->LinearCollidingShapeMask[Entity->UsedLinearIterations] |= CollidingShapeMask;
                        CollidedWith->LinearCollidingShapeMask[CollidedWith->UsedLinearIterations] |= OtherCollidingShapeMask;
#endif
                        ResolveLinearCollision(&Collision, Entity, CollidedWith);
                    }
                }

                CollidedWith = 0;
                Collision.Type = CollisionType_None;
                if(dYaw)
                {
#if COLLISION_DEBUG
                    ++Entity->UsedAngularIterations;
                    Entity->AngularBoundingCircleCollided[Entity->UsedAngularIterations] = false;
                    Entity->AngularCollidingShapeMask[Entity->UsedAngularIterations] = 0;
                    u32 CollidingShapeMask = 0;
                    u32 OtherCollidingShapeMask = 0;
#endif
                    for(u32 OtherEntityIndex = 1;
                        OtherEntityIndex < State->EntityCount;
                        ++OtherEntityIndex)
                    {
                        entity *OtherEntity = State->Entities + OtherEntityIndex;
                        if(!CanCollide(State, Entity, OtherEntity) ||
                           (OtherEntityIndex == EntityIndex)) continue;

                        b32 BoundingBoxesOverlap;
                        {
                            r32 HitRadius = OtherEntity->BoundingRadius + Entity->BoundingRadius;

                            BoundingBoxesOverlap = (HitRadius*HitRadius >=
                                                    LengthSq(OtherEntity->P - Entity->P));
                        }

                        if(BoundingBoxesOverlap)
                        {
#if COLLISION_DEBUG
                            Entity->AngularBoundingCircleCollided[Entity->UsedAngularIterations] = true;
                            OtherEntity->AngularBoundingCircleCollided[Entity->UsedAngularIterations] = true;
                            u32 CollidingShapeMaskIndex = 1;
                            u32 OtherCollidingShapeMaskIndex = 1;
#endif
                            for(collision_shape *Shape = Entity->CollisionShapes;
                                Shape;
                                Shape = Shape->Next)
                            {
                                for(collision_shape *OtherShape = OtherEntity->CollisionShapes;
                                    OtherShape;
                                    OtherShape = OtherShape->Next)
                                {

                                    switch(Shape->Type|OtherShape->Type)
                                    {
                                        case CollisionShapePair_TriangleTriangle:
                                        {
                                            v2 Center = Entity->P.xy;
                                            v2 A = Entity->P.xy + RotateZ(Shape->A, OldYaw);
                                            v2 B = Entity->P.xy + RotateZ(Shape->B, OldYaw);
                                            v2 C = Entity->P.xy + RotateZ(Shape->C, OldYaw);
                                            v2 Centroid = 0.333f*(A + B + C);
                                            r32 ARadius = Length(Shape->A);
                                            r32 BRadius = Length(Shape->B);
                                            r32 CRadius = Length(Shape->C);

                                            v2 OtherA = OtherEntity->P.xy + RotateZ(OtherShape->A, OtherEntity->Yaw);
                                            v2 OtherB = OtherEntity->P.xy + RotateZ(OtherShape->B, OtherEntity->Yaw);
                                            v2 OtherC = OtherEntity->P.xy + RotateZ(OtherShape->C, OtherEntity->Yaw);
                                            v2 OtherCentroid = 0.333f*(OtherA + OtherB + OtherC);
                                            r32 OtherARadius = Length(OtherA-Center);
                                            r32 OtherBRadius = Length(OtherB-Center);
                                            r32 OtherCRadius = Length(OtherC-Center);
                                            
                                            arc_polygon_edge_intersection_result IntersectionAAB =
                                                ArcPolygonEdgeIntersection(Center, ARadius, A, dYaw, OtherA, OtherB, OtherCentroid);
                                            arc_polygon_edge_intersection_result IntersectionABC =
                                                ArcPolygonEdgeIntersection(Center, ARadius, A, dYaw, OtherB, OtherC, OtherCentroid);
                                            arc_polygon_edge_intersection_result IntersectionACA =
                                                ArcPolygonEdgeIntersection(Center, ARadius, A, dYaw, OtherC, OtherA, OtherCentroid);

                                            arc_polygon_edge_intersection_result IntersectionBAB =
                                                ArcPolygonEdgeIntersection(Center, BRadius, B, dYaw, OtherA, OtherB, OtherCentroid);
                                            arc_polygon_edge_intersection_result IntersectionBBC =
                                                ArcPolygonEdgeIntersection(Center, BRadius, B, dYaw, OtherB, OtherC, OtherCentroid);
                                            arc_polygon_edge_intersection_result IntersectionBCA =
                                                ArcPolygonEdgeIntersection(Center, BRadius, B, dYaw, OtherC, OtherA, OtherCentroid);

                                            arc_polygon_edge_intersection_result IntersectionCAB =
                                                ArcPolygonEdgeIntersection(Center, CRadius, C, dYaw, OtherA, OtherB, OtherCentroid);
                                            arc_polygon_edge_intersection_result IntersectionCBC =
                                                ArcPolygonEdgeIntersection(Center, CRadius, C, dYaw, OtherB, OtherC, OtherCentroid);
                                            arc_polygon_edge_intersection_result IntersectionCCA =
                                                ArcPolygonEdgeIntersection(Center, CRadius, C, dYaw, OtherC, OtherA, OtherCentroid);
                                            
                                            arc_polygon_edge_intersection_result OtherIntersectionAAB =
                                                ArcPolygonEdgeIntersection(Center, OtherARadius, OtherA, -dYaw, A, B, Centroid);
                                            arc_polygon_edge_intersection_result OtherIntersectionABC =
                                                ArcPolygonEdgeIntersection(Center, OtherARadius, OtherA, -dYaw, B, C, Centroid);
                                            arc_polygon_edge_intersection_result OtherIntersectionACA =
                                                ArcPolygonEdgeIntersection(Center, OtherARadius, OtherA, -dYaw, C, A, Centroid);

                                            arc_polygon_edge_intersection_result OtherIntersectionBAB =
                                                ArcPolygonEdgeIntersection(Center, OtherBRadius, OtherB, -dYaw, A, B, Centroid);
                                            arc_polygon_edge_intersection_result OtherIntersectionBBC =
                                                ArcPolygonEdgeIntersection(Center, OtherBRadius, OtherB, -dYaw, B, C, Centroid);
                                            arc_polygon_edge_intersection_result OtherIntersectionBCA =
                                                ArcPolygonEdgeIntersection(Center, OtherBRadius, OtherB, -dYaw, C, A, Centroid);

                                            arc_polygon_edge_intersection_result OtherIntersectionCAB =
                                                ArcPolygonEdgeIntersection(Center, OtherCRadius, OtherC, -dYaw, A, B, Centroid);
                                            arc_polygon_edge_intersection_result OtherIntersectionCBC =
                                                ArcPolygonEdgeIntersection(Center, OtherCRadius, OtherC, -dYaw, B, C, Centroid);
                                            arc_polygon_edge_intersection_result OtherIntersectionCCA =
                                                ArcPolygonEdgeIntersection(Center, OtherCRadius, OtherC, -dYaw, C, A, Centroid);

                                            b32 IntersectionAABUpdated = ProcessIntersection(IntersectionAAB, &tMove);
                                            b32 IntersectionABCUpdated = ProcessIntersection(IntersectionABC, &tMove);
                                            b32 IntersectionACAUpdated = ProcessIntersection(IntersectionACA, &tMove);

                                            b32 IntersectionBABUpdated = ProcessIntersection(IntersectionBAB, &tMove);
                                            b32 IntersectionBBCUpdated = ProcessIntersection(IntersectionBBC, &tMove);
                                            b32 IntersectionBCAUpdated = ProcessIntersection(IntersectionBCA, &tMove);

                                            b32 IntersectionCABUpdated = ProcessIntersection(IntersectionCAB, &tMove);
                                            b32 IntersectionCBCUpdated = ProcessIntersection(IntersectionCBC, &tMove);
                                            b32 IntersectionCCAUpdated = ProcessIntersection(IntersectionCCA, &tMove);
                                            
                                            b32 OtherIntersectionAABUpdated = ProcessIntersection(OtherIntersectionAAB, &tMove);
                                            b32 OtherIntersectionABCUpdated = ProcessIntersection(OtherIntersectionABC, &tMove);
                                            b32 OtherIntersectionACAUpdated = ProcessIntersection(OtherIntersectionACA, &tMove);

                                            b32 OtherIntersectionBABUpdated = ProcessIntersection(OtherIntersectionBAB, &tMove);
                                            b32 OtherIntersectionBBCUpdated = ProcessIntersection(OtherIntersectionBBC, &tMove);
                                            b32 OtherIntersectionBCAUpdated = ProcessIntersection(OtherIntersectionBCA, &tMove);

                                            b32 OtherIntersectionCABUpdated = ProcessIntersection(OtherIntersectionCAB, &tMove);
                                            b32 OtherIntersectionCBCUpdated = ProcessIntersection(OtherIntersectionCBC, &tMove);
                                            b32 OtherIntersectionCCAUpdated = ProcessIntersection(OtherIntersectionCCA, &tMove);

                                            
                                            if(IntersectionAABUpdated || IntersectionABCUpdated || IntersectionACAUpdated ||
                                               IntersectionBABUpdated || IntersectionBBCUpdated || IntersectionBCAUpdated ||
                                               IntersectionCABUpdated || IntersectionCBCUpdated || IntersectionCCAUpdated ||
                                               OtherIntersectionAABUpdated || OtherIntersectionABCUpdated || OtherIntersectionACAUpdated ||
                                               OtherIntersectionBABUpdated || OtherIntersectionBBCUpdated || OtherIntersectionBCAUpdated ||
                                               OtherIntersectionCABUpdated || OtherIntersectionCBCUpdated || OtherIntersectionCCAUpdated)
                                            {
                                                CollidedWith = OtherEntity;
#if COLLISION_DEBUG
                                                CollidingShapeMask = CollidingShapeMaskIndex;
                                                OtherCollidingShapeMask = OtherCollidingShapeMaskIndex;
#endif
                                            }
                                        } break;

                                        case CollisionShapePair_CircleCircle:
                                        {
                                            r32 HitRadius = Shape->Radius + OtherShape->Radius;
                                            v2 CenterOffset = RotateZ(Shape->Center, Entity->Yaw);
                                            v2 StartP = Entity->P.xy + CenterOffset;
                                            r32 RotationRadius = Length(Shape->Center);
                                            v2 A = OtherEntity->P.xy +
                                                RotateZ(OtherShape->Center, OtherEntity->Yaw);

                                            arc_circle_intersection_result IntersectionA =
                                                ArcCircleIntersection(Entity->P.xy, RotationRadius, StartP, dYaw, A, HitRadius);

                                            if(ProcessIntersection(IntersectionA, &tMove))
                                            {
                                                CollidedWith = OtherEntity;
#if COLLISION_DEBUG
                                                CollidingShapeMask = CollidingShapeMaskIndex;
                                                OtherCollidingShapeMask = OtherCollidingShapeMaskIndex;
#endif
                                            }
                                        } break;

                                        case CollisionShapePair_TriangleCircle:
                                        {
                                            entity *CircleEntity;
                                            entity *TriangleEntity;
                                            collision_shape *CircleShape;
                                            collision_shape *TriangleShape;
                                            r32 RotationRadius;
                                            v2 CenterOffset;
                                            r32 AdjusteddYaw;
                        
                                            if(Shape->Type == CollisionShapeType_Triangle)
                                            {
                                                TriangleEntity = Entity;
                                                TriangleShape = Shape;
                                                CircleEntity = OtherEntity;
                                                CircleShape = OtherShape;
                                                CenterOffset = RotateZ(CircleShape->Center, CircleEntity->Yaw);
                                                RotationRadius = Length(CircleEntity->P.xy + CenterOffset - TriangleEntity->P.xy);
                                                AdjusteddYaw = -dYaw;
                                            }
                                            else
                                            {
                                                TriangleEntity = OtherEntity;
                                                TriangleShape = OtherShape;
                                                CircleEntity = Entity;
                                                CircleShape = Shape;
                                                CenterOffset = RotateZ(CircleShape->Center, CircleEntity->Yaw);
                                                RotationRadius = Length(CircleShape->Center);
                                                AdjusteddYaw = dYaw;
                                            }
                                            r32 Radius = CircleShape->Radius;
                                            v2 StartP = CircleEntity->P.xy + CenterOffset;
                                            v2 A = TriangleEntity->P.xy + RotateZ(TriangleShape->A, TriangleEntity->Yaw);
                                            v2 B = TriangleEntity->P.xy + RotateZ(TriangleShape->B, TriangleEntity->Yaw);
                                            v2 C = TriangleEntity->P.xy + RotateZ(TriangleShape->C, TriangleEntity->Yaw);
                        
                                            v2 AB = B - A;
                                            v2 BC = C - B;
                                            v2 CA = A - C;
                                            r32 ABLength = Length(AB);
                                            r32 BCLength = Length(BC);
                                            r32 CALength = Length(CA);
                                            r32 InvABLength = 1.0f/ABLength;
                                            r32 InvBCLength = 1.0f/BCLength;
                                            r32 InvCALength = 1.0f/CALength;
                                            v2 ABTranslate = -Perp(AB)*InvABLength*Radius;
                                            v2 BCTranslate = -Perp(BC)*InvBCLength*Radius;
                                            v2 CATranslate = -Perp(CA)*InvCALength*Radius;
                                            v2 ABHitA = A + ABTranslate;
                                            v2 ABHitB = B + ABTranslate;
                                            v2 BCHitB = B + BCTranslate;
                                            v2 BCHitC = C + BCTranslate;
                                            v2 CAHitC = C + CATranslate;
                                            v2 CAHitA = A + CATranslate;

                                            arc_polygon_edge_intersection_result IntersectionAB =
                                                ArcPolygonEdgeIntersection(Entity->P.xy, RotationRadius, StartP, AdjusteddYaw, ABHitA, ABHitB, C);
                                            arc_polygon_edge_intersection_result IntersectionBC =
                                                ArcPolygonEdgeIntersection(Entity->P.xy, RotationRadius, StartP, AdjusteddYaw, BCHitB, BCHitC, A);
                                            arc_polygon_edge_intersection_result IntersectionCA =
                                                ArcPolygonEdgeIntersection(Entity->P.xy, RotationRadius, StartP, AdjusteddYaw, CAHitC, CAHitA, B);

                                            arc_circle_intersection_result IntersectionA =
                                                ArcCircleIntersection(Entity->P.xy, RotationRadius, StartP, AdjusteddYaw, A, Radius);
                                            arc_circle_intersection_result IntersectionB =
                                                ArcCircleIntersection(Entity->P.xy, RotationRadius, StartP, AdjusteddYaw, B, Radius);
                                            arc_circle_intersection_result IntersectionC =
                                                ArcCircleIntersection(Entity->P.xy, RotationRadius, StartP, AdjusteddYaw, C, Radius);

                                            b32 IntersectionABUpdated = ProcessIntersection(IntersectionAB, &tMove);
                                            b32 IntersectionBCUpdated = ProcessIntersection(IntersectionBC, &tMove);
                                            b32 IntersectionCAUpdated = ProcessIntersection(IntersectionCA, &tMove);

                                            b32 IntersectionAUpdated = ProcessIntersection(IntersectionA, &tMove);
                                            b32 IntersectionBUpdated = ProcessIntersection(IntersectionB, &tMove);
                                            b32 IntersectionCUpdated = ProcessIntersection(IntersectionC, &tMove);

                                            if(IntersectionABUpdated || IntersectionBCUpdated || IntersectionCAUpdated ||
                                               IntersectionAUpdated || IntersectionBUpdated || IntersectionCUpdated)
                                            {
                                                CollidedWith = OtherEntity;
#if COLLISION_DEBUG
                                                CollidingShapeMask = CollidingShapeMaskIndex;
                                                OtherCollidingShapeMask = OtherCollidingShapeMaskIndex;
#endif
                                            }
                                        } break;

                                        default:
                                        {
                                            NotImplemented;
                                        } break;
                                    }
#if COLLISION_DEBUG
                                    OtherCollidingShapeMaskIndex = (OtherCollidingShapeMaskIndex << 1);
#endif                                    
                                }
#if COLLISION_DEBUG
                                CollidingShapeMaskIndex = (CollidingShapeMaskIndex << 1);
#endif

                            }
                        }
                    }
                    Entity->Yaw += dYaw*tMove;

                    if(CollidedWith)
                    {
#if COLLISION_DEBUG
                        Entity->AngularCollidingShapeMask[Entity->UsedAngularIterations] |= CollidingShapeMask;
                        CollidedWith->AngularCollidingShapeMask[CollidedWith->UsedAngularIterations] |= OtherCollidingShapeMask;
#endif
                        ResolveAngularCollision(Entity, CollidedWith);
                    }
                }
#if COLLISION_DEBUG
                Entity->CollisionStepP[CollisionIndex] = Entity->P;
                Entity->CollisionStepYaw[CollisionIndex] = Entity->Yaw;
#endif
                tMax -= tMove;
            }
        }
#if COLLISION_FINE_DEBUG
        Entity->P = InitP;
        Entity->dP = InitdP;
        Entity->Yaw = InitYaw;
        Entity->dYaw = InitdYaw;
#endif
    }
    END_TIMED_BLOCK();

#if COLLISION_DEBUG
#define LINEAR_BOUNDING_EXTRUSION_Z_OFFSET -0.0002f
#define LINEAR_BOUNDING_Z_OFFSET -0.0001f
#define LINEAR_SHAPE_EXTRUSION_Z_OFFSET 0.0003f
#define LINEAR_SHAPE_Z_OFFSET 0.0004f
#define ANGULAR_BOUNDING_EXTRUSION_Z_OFFSET -0.0004f
#define ANGULAR_BOUNDING_Z_OFFSET -0.0003f
#define ANGULAR_SHAPE_EXTRUSION_Z_OFFSET 0.0001f
#define ANGULAR_SHAPE_Z_OFFSET 0.0002f
    for(u32 EntityIndex = 1;
        EntityIndex < State->EntityCount;
        ++EntityIndex)
    {
        entity *Entity = State->Entities + EntityIndex;
        if(CanCollide(Entity))
        {
            u32 OldCollidingShapeMask = 0;
            u32 UsedIterations = Maximum(Entity->UsedLinearIterations, Entity->UsedAngularIterations);
            for(u32 CollisionIndex = 0;
                CollisionIndex <= UsedIterations;
                ++CollisionIndex)
            {
                u32 LinearCollidingShapeMask = (OldCollidingShapeMask |
                                                Entity->LinearCollidingShapeMask[CollisionIndex]);
                u32 AngularCollidingShapeMask = (LinearCollidingShapeMask |
                                                 Entity->AngularCollidingShapeMask[CollisionIndex]);
                u32 NewCollidingShapeMask = AngularCollidingShapeMask;
                
                r32 OffsetOffset = 0.00001f*CollisionIndex;
                v3 BoundingExtrusionOffset = V3(0, 0, LINEAR_BOUNDING_EXTRUSION_Z_OFFSET+OffsetOffset);
                v3 BoundingCircleOffset = V3(0, 0, LINEAR_BOUNDING_Z_OFFSET+OffsetOffset);
                v3 ShapeExtrusionOffset = V3(0, 0, LINEAR_SHAPE_EXTRUSION_Z_OFFSET+OffsetOffset);
                v3 ShapeOffset = V3(0, 0, LINEAR_SHAPE_Z_OFFSET+OffsetOffset);

                b32 BoundingCircleCollided = (Entity->LinearBoundingCircleCollided[CollisionIndex] ||
                                              Entity->AngularBoundingCircleCollided[CollisionIndex]);
                v3 P = Entity->CollisionStepP[CollisionIndex];
                r32 Yaw = Entity->CollisionStepYaw[CollisionIndex];
                v4 BoundingColor = BoundingCircleCollided ? V4(1, 0, 0, 1) : V4(1, 0, 1, 1);
                PushCircle(RenderBuffer, P + BoundingCircleOffset, Entity->BoundingRadius, BoundingColor);
                
                v4 ShapeColors[] =
                    {
                        V4(0, 1, 1, 1),
                        V4(0, 0.5, 1, 1),
                    };
                v4 CollidedShapeColors[] =
                    {
                        V4(0, 1, 0, 1),
                        V4(0, 0.5, 0, 1),
                    };
                u32 ShapeColorIndex = 0;
                u32 CollisionShapeMask = 1;
                for(collision_shape *Shape = Entity->CollisionShapes;
                    Shape;
                    Shape = Shape->Next)
                {
                    b32 ShapeCollided = (CollisionShapeMask&NewCollidingShapeMask);
                    v4 ShapeColor = ShapeCollided ?
                        CollidedShapeColors[ShapeColorIndex] :
                        ShapeColors[ShapeColorIndex];
                    switch(Shape->Type)
                    {
                        case CollisionShapeType_Circle:
                        {
                            v3 Center = V3(Shape->Center, 0);
                            v3 RotatedCenter = RotateZ(Center, Yaw);
                            PushCircle(RenderBuffer, P + RotatedCenter  + ShapeOffset,
                                       Shape->Radius, ShapeColor);
                        } break;

                        case CollisionShapeType_Triangle:
                        {
                            v3 A = V3(Shape->A, 0);
                            v3 B = V3(Shape->B, 0);
                            v3 C = V3(Shape->C, 0);
                            A = RotateZ(A, Yaw);
                            B = RotateZ(B, Yaw);
                            C = RotateZ(C, Yaw);
                            PushTriangle(RenderBuffer,
                                         P + A + ShapeOffset,
                                         P + B + ShapeOffset,
                                         P + C + ShapeOffset,
                                         ShapeColor);
                        } break;
                    }
                    ShapeColorIndex = (ShapeColorIndex + 1) & (ArrayCount(ShapeColors)-1);
                    CollisionShapeMask = (CollisionShapeMask << 1);
                }
                
                if(CollisionIndex > 0)
                {
                    v3 OldP = Entity->CollisionStepP[CollisionIndex-1];
                    r32 OldYaw = Entity->CollisionStepYaw[CollisionIndex-1];
                    v3 dP = P - OldP;
                    v4 BoundingExtrusionColor = BoundingCircleCollided ? V4(0.8f, 0, 0, 1) : V4(0.8f, 0, 0.8f, 1);

                    r32 dPLength = Length(dP.xy);
                    r32 InvdPLength = 1.0f / dPLength;
                    v2 XAxis = InvdPLength*-Perp(dP.xy);
                    v2 YAxis = Perp(XAxis);

                    v2 BoundingDim = V2(2.0f*Entity->BoundingRadius, dPLength);
                    PushRotatedRectangle(RenderBuffer, 0.5f*(OldP + P) + BoundingExtrusionOffset,
                                         XAxis, YAxis, BoundingDim, BoundingExtrusionColor);

                    u32 ShapeColorIndex = 0;
                    u32 CollisionShapeMask = 1;
                    for(collision_shape *Shape = Entity->CollisionShapes;
                        Shape;
                        Shape = Shape->Next)
                    {
                        b32 LinearShapeCollided = (CollisionShapeMask&LinearCollidingShapeMask);
                        b32 AngularShapeCollided = (CollisionShapeMask&AngularCollidingShapeMask);
                        v4 LinearShapeColor = LinearShapeCollided ?
                            CollidedShapeColors[ShapeColorIndex] :
                            ShapeColors[ShapeColorIndex];
                        v4 AngularShapeColor = AngularShapeCollided ?
                            CollidedShapeColors[ShapeColorIndex] :
                            ShapeColors[ShapeColorIndex];
                        v4 LinearShapeExtrusionColor = LinearShapeCollided ? V4(0, 0.8f, 0, 1) : V4(0, 0.8f, 0.8f, 1);
                        v4 AngularShapeExtrusionColor = AngularShapeCollided ? V4(0, 0.8f, 0, 1) : V4(0, 0.8f, 0.8f, 1);
                        // TODO(chris): Angular extrusions
                        switch(Shape->Type)
                        {
                            case CollisionShapeType_Circle:
                            {
                                v3 Center = V3(Shape->Center, 0);
                                v3 RotatedOldCenter = RotateZ(Center, OldYaw);
                                v2 ShapeDim = V2(2.0f*Shape->Radius, dPLength);
#if COLLISION_DEBUG_LINEAR
                                PushRotatedRectangle(RenderBuffer,
                                                     RotatedOldCenter + 0.5f*(OldP + P) + ShapeExtrusionOffset,
                                                     XAxis, YAxis, ShapeDim, LinearShapeExtrusionColor);
                                PushCircle(RenderBuffer, P + RotatedOldCenter + ShapeOffset,
                                           Shape->Radius, LinearShapeColor);
#endif
                            } break;

                            case CollisionShapeType_Triangle:
                            {
                                // TODO(chris): Extrude triangles
                                v3 A = V3(Shape->A, 0);
                                v3 B = V3(Shape->B, 0);
                                v3 C = V3(Shape->C, 0);
                                A = RotateZ(A, OldYaw);
                                B = RotateZ(B, OldYaw);
                                C = RotateZ(C, OldYaw);
#if COLLISION_DEBUG_LINEAR
                                PushTriangle(RenderBuffer,
                                             P + A + ShapeOffset,
                                             P + B + ShapeOffset,
                                             P + C + ShapeOffset,
                                             LinearShapeColor);
#endif
                            } break;
                        }
                        ShapeColorIndex = (ShapeColorIndex + 1) & (ArrayCount(ShapeColors)-1);
                        CollisionShapeMask = (CollisionShapeMask << 1);
                    }
                }
                OldCollidingShapeMask = NewCollidingShapeMask;
            }
        }
    }
#endif

    if(Keyboard->ActionUp.EndedDown || ShipController->ActionUp.EndedDown)
    {
        RenderBuffer->CameraP.z += 100.0f*Input->dtForFrame;
    }

    if(Keyboard->ActionDown.EndedDown || ShipController->ActionDown.EndedDown)
    {
        RenderBuffer->CameraP.z -= 100.0f*Input->dtForFrame;
    }

    // NOTE(chris): Post collision pass
    for(u32 EntityIndex = 1;
        EntityIndex < State->EntityCount;
        )
    {
        entity *Entity = State->Entities + EntityIndex;

        if(Entity->Destroyed)
        {
            switch(Entity->Type)
            {
                case EntityType_Ship:
                {
                    --State->Lives;
                    if(State->Lives > 0)
                    {
                        CreateShip(State, State->ShipStartingP, State->ShipStartingYaw);
                    }
                    else
                    {
                        CreateShip(State, V3i(10, 10, 0), State->ShipStartingYaw);
                        GameOver(State, RenderBuffer, &GameMemory->Font);
                    }
                } break;

                case EntityType_Asteroid:
                {
#if 0
                    // NOTE(chris): Halving volume results in dividing radius by cuberoot(2)
                    Entity->Dim *= 0.79370052598f;
                    entity *NewAsteroid = CreateAsteroid(State, Entity->P, Entity->Dim.x);
#endif
                    r32 NewRadius = 0.25f*Entity->Dim.x;
                    if(NewRadius >= 2.0f)
                    {
                        entity *Piece1 = CreateAsteroid(State, Entity->P-V3(NewRadius, 0, 0),
                                                        NewRadius, Entity->dP);
                        entity *Piece2 = CreateAsteroid(State, Entity->P+V3(NewRadius, 0, 0),
                                                        NewRadius, Entity->dP);
                    }
#if 0
                    r32 Angle = Tau/12.0f;
                    Entity->dP = RotateZ(Entity->dP, -Angle);
                    NewAsteroid->dP = RotateZ(NewAsteroid->dP, Angle);
#endif
                } break;

                default:
                {
                } break;
            }
            DestroyEntity(State, Entity);
        }
        else
        {
            ++EntityIndex;
        }
    }

    if(State->Lives <= 0)
    {
        if(WentDown(ShipController->Start))
        {
            ResetGame(State, RenderBuffer, &GameMemory->Font);
        }
    }
    if(WentDown(ShipController->Select))
    {
        GameState->NextMode = GameMode_TitleScreen;
    }
    
    // NOTE(chris): Render pass
    for(u32 EntityIndex = 1;
        EntityIndex < State->EntityCount;
        ++EntityIndex)
    {
        entity *Entity = State->Entities + EntityIndex;
        
        v3 Facing = V3(Cos(Entity->Yaw), Sin(Entity->Yaw), 0.0f);
        v2 YAxis = Facing.xy;
        v2 XAxis = -Perp(YAxis);
        switch(Entity->Type)
        {
            case EntityType_Ship:
            {
//                State->CameraP.xy += 0.2f*(Entity->P.xy - State->CameraP.xy);
//                State->CameraRot += 0.05f*((Entity->Yaw - 0.25f*Tau) - State->CameraRot);
                PushBitmap(RenderBuffer, &GameState->ShipBitmap, Entity->P,
                           XAxis, YAxis, Entity->Dim);
            } break;

            case EntityType_Asteroid:
            {
                PushBitmap(RenderBuffer, &GameState->AsteroidBitmap, Entity->P,
                           XAxis, YAxis, Entity->Dim);
            } break;

            case EntityType_Bullet:
            {
                PushBitmap(RenderBuffer, &GameState->BulletBitmap, Entity->P, XAxis, YAxis,
                           Entity->Dim,
                           V4(1.0f, 1.0f, 1.0f, Unlerp(0.0f, Entity->Timer, 2.0f)));
//            DrawLine(BackBuffer, State->P, Bullet->P, V4(0.0f, 0.0f, 1.0f, 1.0f));
            } break;

            case EntityType_FloatingHead:
            {
#if 0
                for(u32 FaceIndex = 1;
                    FaceIndex < State->HeadMesh.FaceCount;
                    ++FaceIndex)
                {
                    obj_face *Face = State->HeadMesh.Faces + FaceIndex;
                    v3 VertexA = (State->HeadMesh.Vertices + Face->VertexIndexA)->xyz;
                    v3 VertexB = (State->HeadMesh.Vertices + Face->VertexIndexB)->xyz;
                    v3 VertexC = (State->HeadMesh.Vertices + Face->VertexIndexC)->xyz;

                    v4 White = V4(1.0f, 1.0f, 1.0f, 1.0f);
                    PushLine(RenderBuffer, Entity->P, Entity->RotationMatrix,
                             VertexA, VertexB, Entity->Dim, White);
                    PushLine(RenderBuffer, Entity->P, Entity->RotationMatrix,
                             VertexB, VertexC, Entity->Dim, White);
                    PushLine(RenderBuffer, Entity->P, Entity->RotationMatrix,
                             VertexC, VertexA, Entity->Dim, White);
                }
#endif
            } break;

            case EntityType_Letter:
            {
                XAxis = V2(1, 0);
                YAxis = Perp(XAxis);
                loaded_bitmap *Glyph = GameMemory->Font.Glyphs + Entity->Character;
                PushBitmap(RenderBuffer, Glyph, Entity->P,
                           XAxis, YAxis, Entity->Dim);
            } break;

            InvalidDefaultCase;
        }
    }

    {
        RenderBuffer->Projection = Projection_None;
        v2 XAxis = V2(1, 0);
        v2 YAxis = Perp(XAxis);
        v2 Dim = 32.0f*V2(1.0f, 1.0f);
        v2 HalfDim = 0.5f*Dim;
        v3 P = V3(V2i(RenderBuffer->Width, RenderBuffer->Height) - 1.2f*HalfDim, 0);
        for(s32 LifeIndex = 0;
            LifeIndex < State->Lives;
            ++LifeIndex)
        {
            PushBitmap(RenderBuffer, &GameState->ShipBitmap, P, XAxis, YAxis, Dim);
            P.x -= 1.2f*Dim.x;
        }
        
        RenderBuffer->Projection = RenderBuffer->DefaultProjection;
    }

    {
        TIMED_BLOCK("Render Game");
        RenderBufferToBackBuffer(RenderBuffer, BackBuffer);
    }
    EndTemporaryMemory(RenderMemory);
}
