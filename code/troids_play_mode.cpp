/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

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

// TODO(chris): Allow a special shape for triangle fans?
#define CollisionTriangleFan(Name, Fan)                             \
    collision_shape Name[ArrayCount(Fan)-1];                        \
    Name[0]=CollisionTriangle(Fan[0],Fan[ArrayCount(Name)],Fan[1]); \
    for(u32 i=1;i<ArrayCount(Name);++i)                             \
        Name[i]=CollisionTriangle(Fan[0],Fan[i],Fan[i+1]);          \

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

// TODO(chris): Don't need to always allocate this same shape
inline entity *
CreateShip(play_state *State, v3 P, r32 Yaw)
{
    v2 Dim = V2(10.0f, 10.0f);
#if 0
    Dim *= 4.0f;
#endif
    v2 HalfDim = Dim*0.5f;
    collision_shape Shapes[] =
        {
            CollisionTriangle(0.97f*V2(HalfDim.x, 0),
                              0.96f*V2(-HalfDim.x, HalfDim.y),
                              0.69f*V2(-HalfDim.x, 0)),
            CollisionTriangle(0.97f*V2(HalfDim.x, 0),
                              0.69f*V2(-HalfDim.x, 0),
                              0.96f*V2(-HalfDim.x, -HalfDim.y)),
        };

    entity *Result = CreateEntity(State, ArrayCount(Shapes), Shapes);
    Result->Type = EntityType_Ship;
    Result->ColliderType = ColliderType_IndestructibleLaser;
    Result->P = P;
    Result->Yaw = Yaw;
    Result->Timer = 0.0f;

    return(Result);
}

// TODO(chris): Don't need to always allocate this same shape
inline entity *
CreateEnemyShip(play_state *State, v3 P, r32 Yaw, u32 ShipIndex)
{
    entity *Result = CreateShip(State, P, Yaw);
    Result->Type = EntityType_EnemyShip;
    Result->ColliderType = ColliderType_EnemyShip;
    Result->Target = ShipIndex;

    return(Result);
}

inline void
RecenterShapes(entity *Entity)
{
    r32 Count = 0;
    v2 Sum = {};
    for(collision_shape *Shape = Entity->CollisionShapes;
        Shape;
        Shape = Shape->Next)
    {
        switch(Shape->Type)
        {
            case CollisionShapeType_Triangle:
            {
                Sum += Shape->A + Shape->B + Shape->C;
                Count += 3;
            } break;

            InvalidDefaultCase;
        }
    }
    v2 NewCenterOffset = Sum / Count;
    for(collision_shape *Shape = Entity->CollisionShapes;
        Shape;
        Shape = Shape->Next)
    {
        switch(Shape->Type)
        {
            case CollisionShapeType_Triangle:
            {
                Shape->A -= NewCenterOffset;
                Shape->B -= NewCenterOffset;
                Shape->C -= NewCenterOffset;
            } break;

            case CollisionShapeType_Circle:
            {
                Shape->Center -= NewCenterOffset;
            } break;
        }
    }
    Entity->P.xy += RotateZ(NewCenterOffset, Entity->Yaw);
}

inline entity *
CreateAsteroid(play_state *State, r32 MaxRadius, rectangle2 Cell, v3 P = {NAN})
{
    seed *Seed = &State->AsteroidSeed;
    MaxRadius = RandomBetween(Seed, 10.0f, MaxRadius);
    r32 MinRadius = RandomBetween(Seed, 5.0f, MaxRadius - 1.0f);
    v2 Offset = V2(Random01(Seed), Random01(Seed));
    v2 Fan[] =
    {
        Offset,
        Normalize(V2(0.5f, 1.0f)),
        Normalize(V2(-0.5f, 1.0f)),
        Normalize(V2(-1.0f, 0.5f)),
        Normalize(V2(-1.0f, -0.5f)),
        Normalize(V2(-0.5f, -1.0f)),
        Normalize(V2(0.5f, -1.0f)),
        Normalize(V2(1.0f, -0.5f)),
        Normalize(V2(1.0f, 0.5f)),
    };
    for(u32 PointIndex = 1;
        PointIndex < ArrayCount(Fan);
        ++PointIndex)
    {
        Fan[PointIndex] *= RandomBetween(Seed, MinRadius, MaxRadius);
    }
    CollisionTriangleFan(Shapes, Fan);

    entity *Result = CreateEntity(State, ArrayCount(Shapes), Shapes);
    Result->ColliderType = ColliderType_Asteroid;
    Result->Type = EntityType_Asteroid;
    if(IsNaN(P.x))
    {
        Result->P = V3(0.5f*(Cell.Min + Cell.Max), 0.0f);
    }
    else
    {
        Result->P = P;
    }
    Result->dP = V3(RandomBetween(Seed, -20.0f, 20.0f), RandomBetween(Seed, -20.0f, 20.0f), 0.0f);
//    Result->dP = {};
    Result->Yaw = RandomBetween(Seed, 0.0f, Tau);
//    Result->Yaw = 0.0f;
    Result->dYaw = RandomBetween(Seed, -0.5f*Tau, 0.5f*Tau);
//    Result->dYaw = 0.0f;
    
    return(Result);
}

inline entity *
CreateLetter(play_state *State, loaded_font *Font, v3 P, r32 Scale, char Character,
             u32 ShapeCount, collision_shape *Shapes)
{
    loaded_bitmap *Glyph = Font->Glyphs + Character;
    v2 Dim = Scale*V2i(Glyph->Width, Glyph->Height);

    for(u32 ShapeIndex = 0;
        ShapeIndex < ShapeCount;
        ++ShapeIndex)
    {
        collision_shape *Shape = Shapes + ShapeIndex;
        switch(Shape->Type)
        {
            case CollisionShapeType_Triangle:
            {
                Shape->A = Hadamard(Shape->A, Dim);
                Shape->B = Hadamard(Shape->B, Dim);
                Shape->C = Hadamard(Shape->C, Dim);
            } break;

            InvalidDefaultCase;
        }
    }
    
    entity *Result = CreateEntity(State, ShapeCount, Shapes);
    Result->ColliderType = ColliderType_IndestructibleLaser;
    Result->Type = EntityType_Letter;
    Result->P = P;
    Result->Character = Character;
    Result->Dim = Dim;
    Result->Mass = REAL32_MAX;
    Result->Timer = 0.0f;

    return(Result);
}

inline entity *
CreateG(play_state *State, loaded_font *Font, v3 P, r32 Scale)
{
    collision_shape Rectangles[] =
    {
        CollisionRectangle(MinMax(V2(0.547f, 0.384f), V2(1.0f, 0.5f))),
    };
    v2 Strip[] =
    {
        V2(1.0f, 0.384f), V2(0.858f, 0.384f), V2(1.0f, 0.13f), V2(0.858f, 0.2f), V2(0.8f, 0.03f),
        V2(0.68f, 0.12f), V2(0.63f, -0.01f), V2(0.54f, 0.105f), V2(0.45f, -0.01f), V2(0.35f, 0.14f),
        V2(0.26f, 0.05f), V2(0.23f, 0.225f), V2(0.13f, 0.14f), V2(0.16f, 0.35f), V2(0.03f, 0.3f),
        V2(0.14f, 0.43f), V2(0.0f, 0.44f), V2(0.14f, 0.54f), V2(0.001f, 0.59f), V2(0.205f, 0.72f),
        V2(0.066f, 0.75f), V2(0.3f, 0.815f), V2(0.15f, 0.86f), V2(0.4f, 0.855f), V2(0.275f, 0.94f),
        V2(0.55f, 0.875f), V2(0.43f, 0.985f), V2(0.66f, 0.86f), V2(0.65f, 0.985f), V2(0.75f, 0.82f),
        V2(0.81f, 0.93f), V2(0.825f, 0.74f), V2(0.92f, 0.835f), V2(0.85f, 0.67f), V2(0.985f, 0.7f),
    };
    CollisionTriangleStrip(Triangles, Strip);
    CombineShapes(Shapes, Rectangles, Triangles);

    entity *Result = CreateLetter(State, Font, P, Scale, 'G', ArrayCount(Shapes), Shapes);

    return(Result);
}

inline entity *
CreateA(play_state *State, loaded_font *Font, v3 P, r32 Scale)
{
    v2 Strip1[] =
    {
        V2(0.0f, 0.0f), V2(0.158f, 0.0f), V2(0.425f, 1.0f), V2(0.51f, 0.87f), V2(0.575f, 1.0f),
        V2(0.844f, 0.0f), V2(1.0f, 0.0f),
    };
    v2 Strip2[] =
    {
        V2(0.33f, 0.425f), V2(0.28f, 0.304f), V2(0.682f, 0.425f), V2(0.73f, 0.304f),
    };
    CollisionTriangleStrip(Shapes1, Strip1);
    CollisionTriangleStrip(Shapes2, Strip2);
    CombineShapes(Shapes, Shapes1, Shapes2);
    
    entity *Result = CreateLetter(State, Font, P, Scale, 'A', ArrayCount(Shapes), Shapes);

    return(Result);
}

inline entity *
CreateM(play_state *State, loaded_font *Font, v3 P, r32 Scale)
{
    v2 Strip[] =
    {
        V2(0.0f, 0.0f), V2(0.14f, 0.0f), V2(0.0f, 1.0f), V2(0.14f, 0.85f), V2(0.225f, 1.0f),
        V2(0.415f, 0.0f), V2(0.5f, 0.15f), V2(0.585f, 0.0f), V2(0.775f, 1.0f), V2(0.86f, 0.85f),
        V2(1.0f, 1.0f), V2(0.86f, 0.0f), V2(1.0f, 0.0f),
    };
    CollisionTriangleStrip(Shapes, Strip);

    entity *Result = CreateLetter(State, Font, P, Scale, 'M', ArrayCount(Shapes), Shapes);
    
    return(Result);
}

inline entity *
CreateE(play_state *State, loaded_font *Font, v3 P, r32 Scale)
{
    collision_shape Shapes[] =
    {
        CollisionRectangle(MinMax(V2(0.0f, 0.0f), V2(0.18f, 1.0f))),
        CollisionRectangle(MinMax(V2(0.18f, 0.0f), V2(1.0f, 0.12f))),
        CollisionRectangle(MinMax(V2(0.18f, 0.88f), V2(0.965f, 1.0f))),
        CollisionRectangle(MinMax(V2(0.18f, 0.465f), V2(0.915f, 0.586f))),
    };

    entity *Result = CreateLetter(State, Font, P, Scale, 'E', ArrayCount(Shapes), Shapes);

    return(Result);
}

inline entity *
CreateO(play_state *State, loaded_font *Font, v3 P, r32 Scale)
{
    v2 Strip[] =
    {
        V2(0.9f, 0.16f), V2(0.75f, 0.2f), V2(0.77f, 0.05f), V2(0.65f, 0.13f), V2(0.58f, -0.01f),
        V2(0.54f, 0.105f), V2(0.41f, -0.01f), V2(0.33f, 0.14f), V2(0.24f, 0.05f), V2(0.215f, 0.225f),
        V2(0.12f, 0.14f), V2(0.154f, 0.35f), V2(0.025f, 0.3f), V2(0.14f, 0.43f), V2(0.0f, 0.44f),
        V2(0.14f, 0.54f), V2(0.001f, 0.59f), V2(0.195f, 0.72f), V2(0.06f, 0.75f), V2(0.3f, 0.815f),
        V2(0.144f, 0.86f), V2(0.4f, 0.855f), V2(0.27f, 0.94f), V2(0.55f, 0.872f), V2(0.4f, 0.985f),
        V2(0.61f, 0.86f), V2(0.6f, 0.985f), V2(0.7f, 0.82f), V2(0.76f, 0.93f), V2(0.78f, 0.74f),
        V2(0.88f, 0.835f), V2(0.84f, 0.62f), V2(0.962f, 0.7f), V2(0.86f, 0.52f), V2(1.0f, 0.57f),
        V2(0.86f, 0.44f), V2(0.995f, 0.38f), V2(0.84f, 0.34f), V2(0.97f, 0.3f), V2(0.795f, 0.25f),
        V2(0.935f, 0.22f), V2(0.75f, 0.2f), V2(0.9f, 0.16f),
    };
    CollisionTriangleStrip(Shapes, Strip);
    
    entity *Result = CreateLetter(State, Font, P, Scale, 'O', ArrayCount(Shapes), Shapes);

    return(Result);
}

inline entity *
CreateV(play_state *State, loaded_font *Font, v3 P, r32 Scale)
{
    v2 Strip[] =
    {
        V2(1.0f, 1.0f), V2(0.84f, 1.0f), V2(0.58f, 0.0f), V2(0.5f, 0.12f), V2(0.42f, 0.0f),
        V2(0.16f, 1.0f), V2(0.0f, 1.0f),
    };
    CollisionTriangleStrip(Shapes, Strip);
    
    entity *Result = CreateLetter(State, Font, P, Scale, 'V', ArrayCount(Shapes), Shapes);

    return(Result);
}

inline entity *
CreateR(play_state *State, loaded_font *Font, v3 P, r32 Scale)
{
    collision_shape Shapes0[] =
    {
        CollisionRectangle(MinMax(V2(0.0f, 0.0f), V2(0.155f, 1.0f)))
    };
    v2 Strip1[] =
    {
        V2(0.155f, 1.0f), V2(0.155f, 0.88f), V2(0.66f, 1.0f), V2(0.55f, 0.88f), V2(0.77f, 0.965f),
        V2(0.68f, 0.85f), V2(0.87f, 0.88f), V2(0.75f, 0.78f), V2(0.913f, 0.78f), V2(0.76f, 0.7f),
        V2(0.913f, 0.66f), V2(0.7f, 0.6f), V2(0.845f, 0.55f), V2(0.54f, 0.56f), V2(0.73f, 0.48f),
        V2(0.38f, 0.44f), V2(0.6f, 0.44f), V2(0.49f, 0.41f), V2(0.66f, 0.415f), V2(0.53f, 0.38f),
        V2(0.75f, 0.34f), V2(0.82f, 0.0f), V2(1.0f, 0.0f),
    };
    v2 Strip2[] =
    {
        V2(0.155f, 0.56f), V2(0.155f, 0.44f), V2(0.54f, 0.56f), V2(0.38f, 0.44f),
    };
    CollisionTriangleStrip(Shapes1, Strip1);
    CollisionTriangleStrip(Shapes2, Strip2);
    CombineShapes(Combo, Shapes0, Shapes1);
    CombineShapes(Shapes, Combo, Shapes2);

    entity *Result = CreateLetter(State, Font, P, Scale, 'R', ArrayCount(Shapes), Shapes);

    return(Result);
}

// TODO(chris): Don't need to always allocate this same shape
inline entity *
CreateLaser(play_state *State, v3 P, v3 dP, r32 Yaw, r32 Duration)
{
    v2 Dim = V2(1.5f, 3.0f);
    collision_shape Shapes[] =
    {
        CollisionRectangle(CenterDim(V2(0, 0), V2(0.75f*Dim.y, 0.3f*Dim.x))),
    };
    
    entity *Result = CreateEntity(State, ArrayCount(Shapes), Shapes);
    Result->ColliderType = ColliderType_Laser;
    Result->Type = EntityType_Laser;
    Result->P = P;
    Result->dP = dP;
    Result->Yaw = Yaw;
    Result->Dim = Dim;
    Result->Duration = Result->Timer = Duration;
    
    return(Result);
}

inline entity *
CreateEnemyLaser(play_state *State, v3 P, v3 dP, r32 Yaw, r32 Duration)
{
    entity *Result = CreateLaser(State, P, dP, Yaw, Duration);
    Result->ColliderType = ColliderType_EnemyLaser;
    Result->Type = EntityType_EnemyLaser;
    
    return(Result);
}

inline entity *
CreateEnemySpawnTimer(play_state *State, u32 ShipIndex)
{
    entity *Result = CreateEntity(State);
    Result->ColliderType = ColliderType_None;
    Result->Type = EntityType_EnemySpawnTimer;
    Result->Duration = Result->Timer = 3.0f;
    Result->Target = ShipIndex;

    return(Result);
}

inline entity *
CreateMetamorphosisTimer(play_state *State)
{
    entity *Result = CreateEntity(State);
    Result->ColliderType = ColliderType_None;
    Result->Type = EntityType_MetamorphosisTimer;
    Result->Duration = Result->Timer = 3.0f;

    return(Result);
}

inline entity *
CreateEnemyDespawnTimer(play_state *State, entity *Enemy, v4 Color)
{
    entity *Result = CreateEntity(State, Enemy->CollisionShapeCount, Enemy->CollisionShapes);
    Result->ColliderType = ColliderType_None;
    Result->Type = EntityType_EnemyDespawnTimer;
    Result->Duration = Result->Timer = 0.5f;

    for(collision_shape *Shape = Enemy->CollisionShapes;
        Shape;
        Shape = Shape->Next)
    {
        Assert(Shape->Type == CollisionShapeType_Triangle);
        particle *Particle = CreateParticle(State);
        Particle->P = Enemy->P;
        Particle->Yaw = Enemy->Yaw;
        Particle->A = Shape->A;
        Particle->B = Shape->B;
        Particle->C = Shape->C;
        Particle->dP = Enemy->dP;
        Particle->dYaw = Enemy->dYaw;
        Particle->Duration = Particle->Timer = Result->Timer;
        Particle->Color = Color;
    }

    return(Result);
}

inline entity *
CreateSpawnTimer(play_state *State)
{
    entity *Result = CreateEntity(State);
    Result->ColliderType = ColliderType_None;
    Result->Type = EntityType_SpawnTimer;
    Result->Duration = Result->Timer = 2.0f;

    return(Result);
}

inline entity *
CreateShipExplosion(play_state *State, entity *Ship, v4 Color)
{
    entity *Result = CreateEntity(State);
    Result->ColliderType = ColliderType_None;
    Result->Type = EntityType_ShipExplosionTimer;
    Result->Duration = Result->Timer = 2.0f;

    for(collision_shape *Shape = Ship->CollisionShapes;
        Shape;
        Shape = Shape->Next)
    {
        Assert(Shape->Type == CollisionShapeType_Triangle);
        particle *Particle = CreateParticle(State);
        Particle->P = Ship->P;
        Particle->Yaw = Ship->Yaw;
        Particle->A = Shape->A;
        Particle->B = Shape->B;
        Particle->C = Shape->C;
        r32 LargestY = Particle->A.y;
        if(AbsoluteValue(Particle->B.y) > AbsoluteValue(LargestY))
        {
            LargestY = Particle->B.y;
        }
        if(AbsoluteValue(Particle->C.y) > AbsoluteValue(LargestY))
        {
            LargestY = Particle->C.y;
        }
        v3 RandomdP = RotateZ(V3(RandomBetween(&State->ParticleSeed, -1.0f, 1.0f), LargestY, 0.0f),
                              Particle->Yaw);
        Particle->dP = Ship->dP + RandomBetween(&State->ParticleSeed, 2.0f, 10.0f)*Normalize(RandomdP);
        Particle->dYaw = Ship->dYaw + RandomBetween(&State->ParticleSeed, -1.0f, 1.0f);
        Particle->Duration = Particle->Timer = Result->Timer;
        Particle->Color = Color;
    }

    return(Result);
}

inline void
CreateAsteroidExplosion(play_state *State, entity *Asteroid)
{
    for(collision_shape *Shape = Asteroid->CollisionShapes;
        Shape;
        Shape = Shape->Next)
    {
        Assert(Shape->Type == CollisionShapeType_Triangle);
        particle *Particle = CreateParticle(State);
        Particle->P = Asteroid->P;
        Particle->Yaw = Asteroid->Yaw;
        Particle->A = Shape->A;
        Particle->B = Shape->B;
        Particle->C = Shape->C;
        v3 Centroid = V3(0.3333f*(Particle->A + Particle->B + Particle->C), Particle->P.z);
        v3 RandomdP = RandomBetween(&State->ParticleSeed, 5.0f, 10.0f) *
            Normalize(RotateZ(Centroid, Particle->Yaw));
        Particle->dP = Asteroid->dP + RandomdP;
        Particle->dYaw = Asteroid->dYaw + RandomBetween(&State->ParticleSeed, -1.0f, 1.0f);
        Particle->Duration = Particle->Timer = 0.5f;
        Particle->Color = V4(1, 1, 1, 1);
    }
}

inline u32
SplitAsteroid(play_state *State, entity *Asteroid)
{
    b32 CanSplit = Asteroid->CollisionShapeCount > 3;
    
    u32 Result = 0;
    if(Asteroid->DestroyedBy == ColliderType_Laser)
    {
        if(CanSplit)
        {
            Result += 10;
        }
        else
        {
            Result += 10*Asteroid->CollisionShapeCount;
        }
    }

    if((Asteroid->DestroyedBy == ColliderType_IndestructibleLaser) ||
       !CanSplit)
    {
        CreateAsteroidExplosion(State, Asteroid);
        --State->AsteroidCount;
    }
    else
    {
        u32 FirstSplitShapeIndex = RandomBetween(&State->AsteroidSeed,
                                                 2, Asteroid->CollisionShapeCount-2);
        SetNotDestroyed(Asteroid);
        Asteroid->Mass *= 0.5f;
    
        collision_shape *EndShape = Asteroid->CollisionShapes;
        for(u32 ShapeIndex = 1;
            ShapeIndex < FirstSplitShapeIndex;
            ++ShapeIndex)
        {
            EndShape = EndShape->Next;
        }
        entity *Split = CreateEntity(State);
        *Split = *Asteroid;
        Split->CollisionShapes = EndShape->Next;
        Split->CollisionShapeCount = Asteroid->CollisionShapeCount - FirstSplitShapeIndex;
    
        EndShape->Next = 0;
        Asteroid->CollisionShapeCount = FirstSplitShapeIndex;

        RecenterShapes(Asteroid);
        RecenterShapes(Split);

        v3 AsteroidToSplit = Split->P - Asteroid->P;
#if 1
        if(Inner(Asteroid->dP, AsteroidToSplit) > 0.0f)
        {
            Asteroid->dP -= 2.0f*Project(Asteroid->dP, AsteroidToSplit);
        }
        if(Inner(Split->dP, -AsteroidToSplit) > 0.0f)
        {
            Split->dP -= 2.0f*Project(Split->dP, -AsteroidToSplit);
        }
#endif

        r32 InvAsteroidToSplitLength = 1.0f / Length(AsteroidToSplit);
        Asteroid->P -= 0.01f*AsteroidToSplit*InvAsteroidToSplitLength;
        Split->P += 0.01f*AsteroidToSplit*InvAsteroidToSplitLength;
        ++State->AsteroidCount;
    }

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
    Layout.P = ScreenCenter + GetTightAlignmentOffset(GameOverMeasurement);

    v3 P = Unproject(RenderBuffer, V3(Layout.P, 0.0f));

    r32 GameOverWidth = GameOverMeasurement.MaxX - GameOverMeasurement.MinX;
    v3 EndP = Unproject(RenderBuffer, V3(Layout.P + V2(GameOverWidth, 0), 0.0f));
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
ResetField(play_state *State, render_buffer *RenderBuffer, b32 NewGame = false)
{
    State->ResetTimer = 3.0f;
    s32 MinDifficulty = 2;
    s32 MaxDifficulty = 16;
    if(NewGame)
    {
        State->Paused = false;
        State->SourcePoints = 0;
        State->PointsTimer = 0.0f;
        State->Points = 0;
        State->SpinningPoints = 0;
        State->Lives = 3;
        State->Difficulty = MinDifficulty;
        State->EnemyColor = V4(1, 0, 0, 1);
        State->ShipColor = V4(1, 1, 1, 1);
        State->EnemyState = EnemyState_NotHere;
    }
    else
    {
        State->Difficulty *= 2;
    }

    State->ShipColor.r = (r32)RoundS32(State->ShipColor.r);
    State->ShipColor.g = (r32)RoundS32(State->ShipColor.g);
    State->ShipColor.b = (r32)RoundS32(State->ShipColor.b);

    if(State->Difficulty > MaxDifficulty)
    {
        State->Difficulty = MinDifficulty;
    }

    if(State->Difficulty == MaxDifficulty)
    {
        u32 ColorCount = 0;
        v4 Colors[4];
        if(State->ShipColor.r == 1.0f)
        {
            Colors[ColorCount++] = V4(1.0f, 0.0f, 0.0f, 1.0f);
        }
        if(State->ShipColor.g == 1.0f)
        {
            Colors[ColorCount++] = V4(0.0f, 1.0f, 0.0f, 1.0f);
        }
        if(State->ShipColor.b == 1.0f)
        {
            Colors[ColorCount++] = V4(0.0f, 0.0f, 1.0f, 1.0f);
        }
        if(ColorCount)
        {
            State->EnemyColor = Colors[RandomIndex(&State->EnemySeed, ColorCount)];
            State->EnemyState = EnemyState_WaitingToSpawn;
        }
    }
    
    RenderBuffer->CameraRot = 0.0f;

    // NOTE(chris): Null entity
    State->EntityCount = 1;
    State->ParticleCount = 1;
    State->PhysicsState.ShapeArena.Used = 0;
    State->PhysicsState.FirstFreeShape = 0;

    State->ShipStartingP = V3(0.0f, 0.0f, 0.0f);
    State->CameraStartingP = V3(0, 0, 0);
    State->ShipStartingYaw = 0.0f;
    entity Ship_;
    Ship_.BoundingRadius = 0.0f;
    Ship_.P = V3(0.0f, 0.0f, 0.0f);
    entity *Ship = &Ship_;

    
    if(State->ShipColor.rgb == V3(0, 0, 0))
    {
//        State->Difficulty = MaxDifficulty;
        State->Difficulty = 16;
    }
    else
    {
        Ship = CreateShip(State, State->ShipStartingP,
                          State->ShipStartingYaw);
    }

    v3 MinP = Unproject(RenderBuffer, V3(0, 0, 0));
    v3 MaxP = Unproject(RenderBuffer, V3i(RenderBuffer->Width, RenderBuffer->Height, 0));
    r32 MaxRadius = 15.0f;
    r32 MaxCellSize = 2.0f*MaxRadius;
    r32 InvMaxCellSize = 1.0f / MaxCellSize;
    v2 GridDimInMeters = MaxP.xy - MinP.xy;
    v2 GridDimInCells = V2i(Floor(GridDimInMeters.x*InvMaxCellSize),
                            Floor(GridDimInMeters.y*InvMaxCellSize));
    v2 CellDim = V2(GridDimInMeters.x/GridDimInCells.x,
                    GridDimInMeters.y/GridDimInCells.y);

//    Assert(CellDim.x*GridDimInCells.x == GridDimInMeters.x);
//    Assert(CellDim.y*GridDimInCells.y == GridDimInMeters.y);

    s32 CellCount = 0;
    rectangle2 Cells[1024];
    Assert(GridDimInCells.x*GridDimInCells.y <= ArrayCount(Cells));

    r32 HitRadius = Ship->BoundingRadius*2;
    for(s32 CellY = 0;
        CellY < GridDimInCells.y;
        ++CellY)
    {
        for(s32 CellX = 0;
            CellX < GridDimInCells.x;
            ++CellX)
        {
            rectangle2 Cell = MinDim(MinP.xy+V2(CellX*CellDim.x, CellY*CellDim.y),
                                     CellDim);
            rectangle2 HitBox = AddRadius(Cell, HitRadius);
            if(!Inside(HitBox, Ship->P.xy))
            {
                Cells[CellCount++] = Cell;
            }
        }
    }

#if 1
    State->AsteroidCount = State->Difficulty;
    for(s32 AsteroidIndex = 0;
        AsteroidIndex < State->AsteroidCount;
        ++AsteroidIndex)
    {
        u32 CellIndex = RandomIndex(&State->AsteroidSeed, CellCount);
        rectangle2 Cell = Cells[CellIndex];
        Cells[CellIndex] = Cells[--CellCount];
        entity *Asteroid = CreateAsteroid(State, MaxRadius, Cell);
    }
#else
    State->AsteroidCount = 1;
//    CreateAsteroid(State, MaxRadius, Cells[0], MinP)->dP = {};
//    CreateAsteroid(State, MaxRadius, Cells[0], V3(MinP.x, 0, 0))->dP = {};
//    CreateAsteroid(State, MaxRadius, Cells[0], V3(0, MinP.y, 0))->dP = {};
#endif
}

inline void
CalculateVirtualEntities(entity *Entity, r32 dt,
                         rectangle2 FieldRect, virtual_entities *VirtualEntities)
{
    VirtualEntities->P[0] = Entity->P;
    VirtualEntities->Count = 1;

    v3 Dim = V3(GetDim(FieldRect), 0);
    v3 Width = V3(Dim.x, 0, 0);
    v3 Height = V3(0, Dim.y, 0);
    rectangle2 HitBox = AddRadius(FieldRect, -Entity->BoundingRadius);
    v2 OldP = Entity->P.xy;
    v2 NewP = OldP + (Entity->dP.xy + 0.5f*Entity->ddP.xy*dt)*dt;
    rectangle2 dPRect = MinMax(V2(Minimum(OldP.x, NewP.x), Minimum(OldP.y, NewP.y)),
                               V2(Maximum(OldP.x, NewP.x), Maximum(OldP.y, NewP.y)));
    b32 WrapLeft = false;
    b32 WrapRight = false;
    b32 WrapTop = false;
    b32 WrapBottom = false;
    if(dPRect.Max.x > HitBox.Max.x)
    {
        WrapRight = true;
    }
    if(dPRect.Min.x < HitBox.Min.x)
    {
        WrapLeft = true;
    }
    if(dPRect.Max.y > HitBox.Max.y)
    {
        WrapTop = true;
    }
    if(dPRect.Min.y < HitBox.Min.y)
    {
        WrapBottom = true;
    }

    Assert(!(WrapTop && WrapBottom));
    Assert(!(WrapLeft && WrapRight));

    if(WrapRight)
    {
        Assert(VirtualEntities->Count < ArrayCount(VirtualEntities->P));
        VirtualEntities->P[VirtualEntities->Count++] = Entity->P - Width;
    }
    else if(WrapLeft)
    {
        Assert(VirtualEntities->Count < ArrayCount(VirtualEntities->P));
        VirtualEntities->P[VirtualEntities->Count++] = Entity->P + Width;
    }
    if(WrapTop)
    {
        Assert(VirtualEntities->Count < ArrayCount(VirtualEntities->P));
        VirtualEntities->P[VirtualEntities->Count++] = Entity->P - Height;
    }
    else if(WrapBottom)
    {
        Assert(VirtualEntities->Count < ArrayCount(VirtualEntities->P));
        VirtualEntities->P[VirtualEntities->Count++] = Entity->P + Height;
    }

    if(WrapRight && WrapTop)
    {
        Assert(VirtualEntities->Count < ArrayCount(VirtualEntities->P));
        VirtualEntities->P[VirtualEntities->Count++] = Entity->P - Dim;
    }
    else if(WrapRight && WrapBottom)
    {
        Assert(VirtualEntities->Count < ArrayCount(VirtualEntities->P));
        VirtualEntities->P[VirtualEntities->Count++] = Entity->P + V3(-Dim.x, Dim.y, 0);
    }
    else if(WrapLeft && WrapTop)
    {
        Assert(VirtualEntities->Count < ArrayCount(VirtualEntities->P));
        VirtualEntities->P[VirtualEntities->Count++] = Entity->P + V3(Dim.x, -Dim.y, 0);
    }
    else if(WrapLeft && WrapBottom)
    {
        Assert(VirtualEntities->Count < ArrayCount(VirtualEntities->P));
        VirtualEntities->P[VirtualEntities->Count++] = Entity->P + Dim;
    }
}

inline void
PushWraparoundTriangle(render_buffer *RenderBuffer, rectangle2 FieldRect,
                         v3 A, v3 B, v3 C, v4 Color)
{
    PushTriangle(RenderBuffer, A, B, C, Color);
    b32 WrapRight = (A.x > FieldRect.Max.x) || (B.x > FieldRect.Max.x) || (C.x > FieldRect.Max.x);
    b32 WrapLeft = (A.x < FieldRect.Min.x) || (B.x < FieldRect.Min.x) || (C.x < FieldRect.Min.x);
    b32 WrapTop = (A.y > FieldRect.Max.y) || (B.y > FieldRect.Max.y) || (C.y > FieldRect.Max.y);
    b32 WrapBottom = (A.y < FieldRect.Min.y) || (B.y < FieldRect.Min.y) || (C.y < FieldRect.Min.y);
    v2 FieldDim = GetDim(FieldRect);
    if(WrapRight)
    {
        v3 Offset = V3(-FieldDim.x, 0, 0);
        PushTriangle(RenderBuffer,
                     A + Offset,
                     B + Offset,
                     C + Offset,
                     Color);
    }
    if(WrapLeft)
    {
        v3 Offset = V3(FieldDim.x, 0, 0);
        PushTriangle(RenderBuffer,
                     A + Offset,
                     B + Offset,
                     C + Offset,
                     Color);
    }
    if(WrapTop)
    {
        v3 Offset = V3(0, -FieldDim.y, 0);
        PushTriangle(RenderBuffer,
                     A + Offset,
                     B + Offset,
                     C + Offset,
                     Color);
    }
    if(WrapBottom)
    {
        v3 Offset = V3(0, FieldDim.y, 0);
        PushTriangle(RenderBuffer,
                     A + Offset,
                     B + Offset,
                     C + Offset,
                     Color);
    }
    if(WrapRight && WrapTop)
    {
        v3 Offset = V3(-FieldDim.x, -FieldDim.y, 0);
        PushTriangle(RenderBuffer,
                     A + Offset,
                     B + Offset,
                     C + Offset,
                     Color);
    }
    if(WrapLeft && WrapTop)
    {
        v3 Offset = V3(FieldDim.x, -FieldDim.y, 0);
        PushTriangle(RenderBuffer,
                     A + Offset,
                     B + Offset,
                     C + Offset,
                     Color);
    }
    if(WrapRight && WrapBottom)
    {
        v3 Offset = V3(-FieldDim.x, FieldDim.y, 0);
        PushTriangle(RenderBuffer,
                     A + Offset,
                     B + Offset,
                     C + Offset,
                     Color);
    }
    if(WrapLeft && WrapBottom)
    {
        v3 Offset = V3(FieldDim.x, FieldDim.y, 0);
        PushTriangle(RenderBuffer,
                     A + Offset,
                     B + Offset,
                     C + Offset,
                     Color);
    }
}

internal void
PlayMode(game_memory *GameMemory, game_input *Input, renderer_state *RendererState)
{
    game_state *GameState = (game_state *)GameMemory->PermanentMemory;
    transient_state *TranState = (transient_state *)GameMemory->TemporaryMemory;
    play_state *State = &GameState->PlayState;

    render_buffer *RenderBuffer = &TranState->RenderBuffer;
    RenderBuffer->Projection = RenderBuffer->DefaultProjection;
    
    // NOTE(chris): Targeting 60Hz
    r32 dtPhysics = 1.0f/60.0f;
    
    if(!State->IsInitialized)
    {
        State->IsInitialized = true;

        State->AsteroidSeed = Seed(Input->SeedValue);
        State->EngineSeed = Seed(Input->SeedValue);
        State->ParticleSeed = Seed(Input->SeedValue);
        State->EnemySeed = Seed(Input->SeedValue);

        InitializePhysicsState(&State->PhysicsState, &GameState->Arena);

        AllowCollision(&State->PhysicsState, ColliderType_Ship, ColliderType_Asteroid);
        AllowCollision(&State->PhysicsState, ColliderType_Ship, ColliderType_EnemyShip);
        AllowCollision(&State->PhysicsState, ColliderType_Ship, ColliderType_EnemyLaser);
        AllowCollision(&State->PhysicsState, ColliderType_Laser, ColliderType_EnemyShip);
        AllowCollision(&State->PhysicsState, ColliderType_Laser, ColliderType_Asteroid);
        AllowCollision(&State->PhysicsState, ColliderType_Asteroid, ColliderType_Asteroid);
//        AllowCollision(&State->PhysicsState, ColliderType_Ship, ColliderType_Wall);
        AllowCollision(&State->PhysicsState, ColliderType_Asteroid, ColliderType_Wall);
        AllowCollision(&State->PhysicsState, ColliderType_Asteroid, ColliderType_IndestructibleLaser);

        RenderBuffer->CameraP = V3(0.0f, 0.0f, 150.0f);
        ResetField(State, RenderBuffer, true);
    }

    if(State->ShipColor.rgb == V3(0, 0, 0))
    {
        if(RenderBuffer->CameraP.z >= 25000.0f)
        {
            GameState->NextMode = GameMode_TitleScreen;
        }
        else
        {
            RenderBuffer->CameraP.z *= 1.002f;
        }
    }
    
    temporary_memory RenderMemory = BeginTemporaryMemory(&RenderBuffer->Arena);
    PushClear(RendererState, RenderBuffer, V3(0.0f, 0.0f, 0.0f));
    v2 ScreenCenter = 0.5f*V2i(RenderBuffer->Width, RenderBuffer->Height); 

#if DEBUG_CAMERA
    RenderBuffer->ClipCameraP = V3(State->CameraStartingP.xy, 200.0f);
    RenderBuffer->CameraP = V3(State->CameraStartingP.xy, 300.0f);
    v3 UnprojectedMin = Unproject(RenderBuffer, V3(0.0f, 0.0f, 0.0f), RenderBuffer->ClipCameraP);
    v3 UnprojectedMax = Unproject(RenderBuffer, V3i(RenderBuffer->Width, RenderBuffer->Height, 0),
                                  RenderBuffer->ClipCameraP);
    RenderBuffer->ClipRect = MinMax(Project(RenderBuffer, UnprojectedMin).xy,
        Project(RenderBuffer, UnprojectedMax).xy);
    PushBorder(RenderBuffer,
               UnprojectedMin,
               (UnprojectedMax-UnprojectedMin).xy,
               V4(1, 0, 0, 1));
#endif
        
    rectangle2 FieldRect = MinMax(Unproject(RenderBuffer, V3(0, 0, 0)).xy,
                                  Unproject(RenderBuffer,
                                            V3i(RenderBuffer->Width, RenderBuffer->Height, 0)).xy);
    v2 FieldDim = GetDim(FieldRect);
    v3 FieldWidthOffset = V3(FieldDim.x, 0, 0);
    v3 FieldHeightOffset = V3(0, FieldDim.y, 0);

    game_controller *ShipController = Input->Controllers + Input->MostRecentlyUsedController;
    if(!State->Paused)
    {
        virtual_entities VirtualEntityTable[ArrayCount(State->Entities)];
    
        // NOTE(chris): Particle update pass
        for(u32 ParticleIndex = 1;
            ParticleIndex < State->ParticleCount;
            )
        {
            u32 NextIndex = ParticleIndex + 1;
            particle *Particle = State->Particles + ParticleIndex;
            if(Particle->Timer <= 0.0f)
            {
                DestroyParticle(State, Particle);
                NextIndex = ParticleIndex;
            }
            else
            {
                Particle->P += Particle->dP*Input->dtForFrame;
                if(Particle->P.x > FieldRect.Max.x)
                {
                    Particle->P.x -= FieldDim.x;
                }
                else if(Particle->P.x < FieldRect.Min.x)
                {
                    Particle->P.x += FieldDim.x;
                }
                if(Particle->P.y > FieldRect.Max.y)
                {
                    Particle->P.y -= FieldDim.y;
                }
                else if(Particle->P.y < FieldRect.Min.y)
                {
                    Particle->P.y += FieldDim.y;
                }
                Particle->Yaw += Particle->dYaw*Input->dtForFrame;
                Particle->Timer -= Input->dtForFrame;
            }

            ParticleIndex = NextIndex;
        }
        
#if COLLISION_DEBUG
        b32 FirstAsteroid = true;
        game_controller *AsteroidController = Input->GamePads + 1;
#endif

        r32 MaxdYawPerStep = 0.49f*Tau;
        r32 MaxdYawPerSecond = MaxdYawPerStep/dtPhysics;

        u32 PhysicsIterations = RoundU32(Input->dtForFrame/dtPhysics);

        // NOTE(chris): Everything in here must use dtPhysics as it is all being subdivided.
        // TODO(chris): I could probably do most of this in a different update loop and just pull the
        // controller code into the physics iterations to make sure all accelerations are being applied
        // at the right time.
        for(u32 TimestepIndex = 0;
            TimestepIndex < PhysicsIterations;
            ++TimestepIndex)
        {
            // NOTE(chris): Pre collision pass
            for(u32 EntityIndex = 1;
                EntityIndex < State->EntityCount;
                )
            {
                u32 NextIndex = EntityIndex + 1;
                entity *Entity = State->Entities + EntityIndex;

                if(Entity->P.x > FieldRect.Max.x)
                {
                    Entity->P.x -= FieldDim.x;
                }
                if(Entity->P.x < FieldRect.Min.x)
                {
                    Entity->P.x += FieldDim.x;
                }
                if(Entity->P.y > FieldRect.Max.y)
                {
                    Entity->P.y -= FieldDim.y;
                }
                if(Entity->P.y < FieldRect.Min.y)
                {
                    Entity->P.y += FieldDim.y;
                }

                v3 Facing = V3(Cos(Entity->Yaw), Sin(Entity->Yaw), 0.0f);
                switch(Entity->Type)
                {
                    case EntityType_Ship:
                    {
                        if(State->AsteroidCount <= 0 && State->EnemyState == EnemyState_WaitingToSpawn)
                        {
                            State->EnemyState = EnemyState_Here;
                            CreateEnemySpawnTimer(State, EntityIndex);
                        }
                        if((Entity->ColliderType == ColliderType_IndestructibleLaser) &&
                           (Entity->Timer == 0.0f))
                        {
                            Entity->Timer += dtPhysics;
                        }
                        else
                        {
                            if(Entity->ColliderType == ColliderType_IndestructibleLaser)
                            {
                                Entity->Timer = 0.0f;
                                Entity->ColliderType = ColliderType_Ship;
                            }
#if TROIDS_INTERNAL
                            if(ShipController->LeftClick.EndedDown)
                            {
                                Entity->P = State->ShipStartingP;
                            }
#endif
                            r32 LaserSpeed = 100.0f;
                            r32 LaserDuration = 1.0f;
                    
                            r32 Thrust = Clamp01(ShipController->LeftStick.y);
                            ShipController->LowFrequencyMotor = Thrust;

#if COLLISION_FINE_DEBUG
                            Entity->ddYaw = -100.0f*ShipController->LeftStick.x;
#else
                            Entity->ddYaw = -2.0f*ShipController->LeftStick.x;
#endif
    
                            Entity->ddP = {};
                            if(ShipController->ActionDown.EndedDown)
                            {
                                if(Entity->Timer <= 0.0f)
                                {
                                    entity *Laser = CreateLaser(State, Entity->P,
                                                                Entity->dP + Facing*LaserSpeed,
                                                                Entity->Yaw, LaserDuration);
                                    r32 LaserOffset = 0.5f*(Entity->Dim.y + Laser->Dim.y);
                                    Laser->P += Facing*(LaserOffset);
                                    Entity->Timer = 0.1f;
                                    Entity->ddP += -Facing*100.0f;
                                }
                            }
#if TROIDS_INTERNAL
                            if(WentDown(ShipController->ActionRight))
                            {
                                Entity->DestroyedBy = ColliderType_Asteroid;
                            }
                            Entity->ddP += V3(1000.0f*ShipController->RightStick, 0);
#endif

#if COLLISION_FINE_DEBUG
                            Entity->ddP += 5000.0f*Facing*Thrust;
#else
                            Entity->ddP += 50.0f*Facing*Thrust;
#endif
                            Entity->Flicker = RandomBetween(&State->EngineSeed, 0.0f, Thrust);

                            if(Entity->Timer > 0.0f)
                            {
                                Entity->Timer -= dtPhysics;
                                ShipController->HighFrequencyMotor = 1.0f;
                            }
                            else
                            {
                                ShipController->HighFrequencyMotor = 0.0f;
                            }
                        }
                    } break;
            
                    case EntityType_EnemyShip:
                    {
                        entity *Ship = State->Entities + Entity->Target;
                        if(Ship->Type == EntityType_Ship && !IsDestroyed(Ship))
                        {
                            b32 Fire = false;

                            r32 LaserSpeed = 100.0f;
                            r32 LaserDuration = 1.0f;
                            r32 LaserDistance = LaserSpeed*LaserDuration;
                            v3 ToShip = Ship->P - Entity->P;
                            v3 NormToShip = Normalize(ToShip);
                            r32 FacingShip = Inner(Facing.xy, NormToShip.xy);
                            r32 LeftStickX = 0.0f;
                            r32 Thrust = 0.0f;
                            if(FacingShip >= 0.0f)
                            {
                                v2 NormPerp = Normalize(-Perp(Facing.xy));
                                r32 InnerShip = Inner(NormPerp, NormToShip.xy);
                                LeftStickX = Sign(InnerShip)*Cube(AbsoluteValue(InnerShip));
                                Thrust = Cube(FacingShip);
                                Fire = Length(ToShip) <= LaserDistance;
                            }
                            else
                            {
                                v2 NormPerp = Normalize(Perp(Facing.xy));
                                r32 InnerShip = Inner(NormPerp, NormToShip.xy);
                                LeftStickX = -Sign(InnerShip)*(1-0.5f*AbsoluteValue(InnerShip));
                            }

                            Entity->ddYaw = -2.0f*LeftStickX;
    
                            Entity->ddP = {};
                            if(Fire && Entity->Timer <= 0.0f)
                            {
                                entity *Laser = CreateEnemyLaser(State, Entity->P, Entity->dP + Facing*LaserSpeed,
                                                                 Entity->Yaw, LaserDuration);
                                r32 LaserOffset = 0.5f*(Entity->Dim.y + Laser->Dim.y);
                                Laser->P += Facing*(LaserOffset);
                                Entity->Timer = 0.1f;
                                Entity->ddP += -Facing*100.0f;
                            }

                            Entity->ddP += 50.0f*Facing*Thrust;
                            Entity->Flicker = RandomBetween(&State->EngineSeed, 0.0f, Thrust);

                            if(Entity->Timer > 0.0f)
                            {
                                Entity->Timer -= dtPhysics;
                                ShipController->HighFrequencyMotor = 1.0f;
                            }
                            else
                            {
                                ShipController->HighFrequencyMotor = 0.0f;
                            }

#if TROIDS_INTERNAL
                            if(WentDown(ShipController->ActionLeft))
                            {
                                Entity->DestroyedBy = ColliderType_Ship;
                            }
#endif
                        }
                        else
                        {
                            if(State->PlayType == PlayType_Journey)
                            {
                                State->EnemyState = EnemyState_WaitingToSpawn;
                            }
                            else
                            {
                                State->EnemyState = EnemyState_NotHere;
                            }
                            CreateEnemyDespawnTimer(State, Entity, State->EnemyColor);
                            DestroyEntity(State, Entity);
                            NextIndex = EntityIndex;
                        }
                    } break;

                    case EntityType_Asteroid:
                    {
#if COLLISION_DEBUG
                        if(FirstAsteroid)
                        {
                            FirstAsteroid = false;
    
                            Entity->ddP = 100.0f*V3(AsteroidController->LeftStick, 0);
                        }
#endif
                    } break;


                    case EntityType_EnemyDespawnTimer:
                    case EntityType_Laser:
                    case EntityType_EnemyLaser:
                    {
                        if(Entity->Timer <= 0.0f)
                        {
                            DestroyEntity(State, Entity);
                            NextIndex = EntityIndex;
                        }
                        else
                        {
                            Entity->Timer -= dtPhysics;
                        }
                    } break;

                    case EntityType_Letter:
                    {
                        if(Entity->Timer == 0.0f)
                        {
                            Entity->Timer += dtPhysics;
                        }
                        else
                        {
                            Entity->ColliderType = ColliderType_Wall;
                        }
                    } break;

                    case EntityType_SpawnTimer:
                    {
                        if(Entity->Timer <= 0.0f)
                        {
                            if(State->AsteroidCount || State->EnemyState == EnemyState_WaitingToSpawn)
                            {
                                entity *Ship = CreateShip(State, State->ShipStartingP, State->ShipStartingYaw);
                            }
                            DestroyEntity(State, Entity);
                            NextIndex = EntityIndex;
                        }
                        else
                        {
                            Entity->Timer -= dtPhysics;
                        }
                    } break;

                    case EntityType_MetamorphosisTimer:
                    {
                        if(Entity->Timer <= 0.0f)
                        {
                            DestroyEntity(State, Entity);
                            NextIndex = EntityIndex;
                        }
                        else
                        {
                            State->ShipColor.rgb -= State->EnemyColor.rgb*dtPhysics/Entity->Duration;
                            State->ShipColor.r = Clamp01(State->ShipColor.r);
                            State->ShipColor.g = Clamp01(State->ShipColor.g);
                            State->ShipColor.b = Clamp01(State->ShipColor.b);
                            Entity->Timer -= dtPhysics;
                        }
                    } break;

                    case EntityType_EnemySpawnTimer:
                    {
                        entity *Ship = State->Entities + Entity->Target;
                        Assert(Ship->Type == EntityType_Ship && !IsDestroyed(Ship));
                        Entity->Yaw = Ship->Yaw;
                        Entity->P = V3(Ship->P.xy + 0.5f*FieldDim, Ship->P.z);
                        if(Entity->P.x > FieldRect.Max.x)
                        {
                            Entity->P.x -= FieldDim.x;
                        }
                        if(Entity->P.y > FieldRect.Max.y)
                        {
                            Entity->P.y -= FieldDim.y;
                        }
                
                        if(Entity->Timer <= 0.0f)
                        {
                            entity *EnemyShip = CreateEnemyShip(State, Entity->P, Entity->Yaw, Entity->Target);
                            EnemyShip->dP = Ship->dP;
                            EnemyShip->dYaw = Ship->dYaw;
                            DestroyEntity(State, Entity);
                            NextIndex = EntityIndex;
                        }
                        else
                        {
                            r32 Thrust = Clamp01(ShipController->LeftStick.y);
                            Entity->Flicker = RandomBetween(&State->EngineSeed, 0.0f, Thrust);
                    
                            Entity->Timer -= dtPhysics;
                        }
                    } break;

                    case EntityType_ShipExplosionTimer:
                    {
                        if(Entity->Timer <= 0.0f)
                        {
                            DestroyEntity(State, Entity);
                            NextIndex = EntityIndex;
                            ShipController->LowFrequencyMotor = 0.0f;
                            ShipController->HighFrequencyMotor = 0.0f;
                        }
                        else
                        {
                            ShipController->HighFrequencyMotor =
                                ShipController->LowFrequencyMotor = Clamp01(Entity->Timer / Entity->Duration);
                            Entity->Timer -= dtPhysics;
                        }
                    } break;

                    default:
                    {
                    } break;
                }
                if(CanCollide(Entity))
                {
#if COLLISION_DEBUG

                    Entity->UsedLinearIterations = 0;
                    Entity->UsedAngularIterations = 0;

                    Entity->CollisionStepP[0] = Entity->P;
                    Entity->LinearBoundingCircleCollided[0] = false;
                    Entity->LinearCollidingShapeMask[0] = 0;
                    Entity->CollisionStepYaw[0] = Entity->Yaw;
                    Entity->AngularBoundingCircleCollided[0] = false;
                    Entity->AngularCollidingShapeMask[0] = 0;
#if COLLISION_FINE_DEBUG
                    Entity->InitdP = Entity->dP;
                    Entity->InitdYaw = Entity->dYaw;
#endif
#endif
                }

                virtual_entities *VirtualEntities = VirtualEntityTable + EntityIndex;

                CalculateVirtualEntities(Entity, dtPhysics, FieldRect, VirtualEntities);
                Entity->Yaw = RealMod(Entity->Yaw, Tau);
                Assert(0 <= Entity->Yaw && Entity->Yaw <= Tau);

                EntityIndex = NextIndex;
            }
        
            BEGIN_TIMED_BLOCK(Collision, "Collision");
            // NOTE(chris): Collision pass
            for(u32 EntityIndex = 1;
                EntityIndex < State->EntityCount;
                ++EntityIndex)
            {
                entity *Entity_ = State->Entities + EntityIndex;
                virtual_entities *VirtualEntities = VirtualEntityTable + EntityIndex;
                if(CanCollide(Entity_))
                {
                    r32 tMax = 1.0f;
                    for(u32 CollisionIndex = 1;
                        CollisionIndex <= COLLISION_ITERATIONS;
                        ++CollisionIndex)
                    {
                        r32 tMove = tMax;
                        // TODO(chris): IMPORTANT clamp to max speed!
                        v3 dP = (Entity_->dP + 0.5f*Entity_->ddP*dtPhysics)*dtPhysics;
                        r32 dYaw = (Entity_->dYaw + 0.5f*Entity_->ddYaw*dtPhysics)*dtPhysics;
                        dYaw = Clamp(-MaxdYawPerSecond, dYaw, MaxdYawPerSecond);

                        if((tMax <= 0.0f) || (dP.x == 0 && dP.y == 0 && dYaw == 0)) break;
                        if(dP.x != 0 || dP.y != 0)
                        {
                            entity *CollidedWith = 0;
                            collision Collision = {};
                            for(u32 VirtualEntityIndex = 0;
                                VirtualEntityIndex < VirtualEntities->Count;
                                ++VirtualEntityIndex)
                            {
                                v3 OldP = VirtualEntities->P[VirtualEntityIndex];
                                v3 NewP = OldP + dP;

#if COLLISION_DEBUG
                                // TODO(chris): IMPORTANT All this stuff breaks because of virtual entities
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
                                    entity *OtherEntity_ = State->Entities + OtherEntityIndex;
                                    if(!CanCollide(State, Entity_, OtherEntity_) ||
                                       (OtherEntityIndex == EntityIndex)) continue;

                                    virtual_entities *OtherVirtualEntities = VirtualEntityTable + OtherEntityIndex;
                                    for(u32 OtherVirtualEntityIndex = 0;
                                        OtherVirtualEntityIndex < OtherVirtualEntities->Count;
                                        ++OtherVirtualEntityIndex)
                                    {
                                        v3 OtherP = OtherVirtualEntities->P[OtherVirtualEntityIndex];
                                        if(BoundingCirclesIntersect(OldP, NewP, Entity_->BoundingRadius,
                                                                    OtherP, OtherEntity_->BoundingRadius))
                                        {
#if COLLISION_DEBUG
                                            Entity->LinearBoundingCircleCollided[Entity_->UsedLinearIterations] = true;
                                            OtherEntity->LinearBoundingCircleCollided[OtherEntity_->UsedLinearIterations] = true;
                                            u32 CollidingShapeMaskIndex = 1;
                                            u32 OtherCollidingShapeMaskIndex = 1;
#endif
                                            for(collision_shape *Shape = Entity_->CollisionShapes;
                                                Shape;
                                                Shape = Shape->Next)
                                            {
                                                for(collision_shape *OtherShape = OtherEntity_->CollisionShapes;
                                                    OtherShape;
                                                    OtherShape = OtherShape->Next)
                                                {
                                                    switch(Shape->Type|OtherShape->Type)
                                                    {
                                                        case CollisionShapePair_TriangleTriangle:
                                                        {
                                                            v2 Offset = OtherP.xy;
                                            
                                                            v2 OtherRotatedA = RotateZ(OtherShape->A, OtherEntity_->Yaw);
                                                            v2 OtherRotatedB = RotateZ(OtherShape->B, OtherEntity_->Yaw);
                                                            v2 OtherRotatedC = RotateZ(OtherShape->C, OtherEntity_->Yaw);
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

                                                            v2 RotatedA = RotateZ(Shape->A, Entity_->Yaw);
                                                            v2 RotatedB = RotateZ(Shape->B, Entity_->Yaw);
                                                            v2 RotatedC = RotateZ(Shape->C, Entity_->Yaw);
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

                                                                CollidedWith = OtherEntity_;
#if COLLISION_DEBUG
                                                                CollidingShapeMask = CollidingShapeMaskIndex;
                                                                OtherCollidingShapeMask = OtherCollidingShapeMaskIndex;
#endif
                                                            }
                                                        } break;

                                                        case CollisionShapePair_CircleCircle:
                                                        {
                                                            v2 Start = OldP.xy + RotateZ(Shape->Center, Entity_->Yaw);
                                                            v2 End = Start + dP.xy;
                                                            v2 dMove = End - Start;
                                                            r32 HitRadius = Shape->Radius + OtherShape->Radius;
                                                            v2 HitCenter = (OtherP.xy +
                                                                            RotateZ(OtherShape->Center, OtherEntity_->Yaw));

                                                            circle_ray_intersection_result Intersection =
                                                                CircleRayIntersection(HitCenter, HitRadius, Start, End);

                                                            if(ProcessIntersection(Intersection, &tMove))
                                                            {
                                                                Collision.Type = CollisionType_Circle;
                                                                Collision.Deflection = Start - HitCenter;

                                                                CollidedWith = OtherEntity_;
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
                                                                Start = OtherP.xy + RotateZ(OtherShape->Center, OtherEntity_->Yaw);
                                                                End = Start - dP.xy;
                                                                Radius = OtherShape->Radius;
                                                                A = OldP.xy + RotateZ(Shape->A, Entity_->Yaw);
                                                                B = OldP.xy + RotateZ(Shape->B, Entity_->Yaw);
                                                                C = OldP.xy + RotateZ(Shape->C, Entity_->Yaw);
                                                            }
                                                            else
                                                            {
                                                                Start = OldP.xy + RotateZ(Shape->Center, Entity_->Yaw);
                                                                End = Start + dP.xy;
                                                                Radius = Shape->Radius;
                                                                A = OtherP.xy +
                                                                    RotateZ(OtherShape->A, OtherEntity_->Yaw);
                                                                B = OtherP.xy +
                                                                    RotateZ(OtherShape->B, OtherEntity_->Yaw);
                                                                C = OtherP.xy +
                                                                    RotateZ(OtherShape->C, OtherEntity_->Yaw);
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

                                                                CollidedWith = OtherEntity_;
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
                                }
                            }
                            Entity_->P += dP*tMove;
                            Entity_->dP += Entity_->ddP*dtPhysics*tMove;
                            // TODO(chris): This is to clear out player input. How to keep around other forces?
                            Entity_->ddP = {};
                            CalculateVirtualEntities(Entity_, (1.0f-tMove)*dtPhysics, FieldRect, VirtualEntities);
                    
                            if(CollidedWith)
                            {
#if COLLISION_DEBUG
                                Entity->LinearCollidingShapeMask[Entity->UsedLinearIterations] |= CollidingShapeMask;
                                CollidedWith->LinearCollidingShapeMask[CollidedWith->UsedLinearIterations] |= OtherCollidingShapeMask;
#endif
                                ResolveLinearCollision(&Collision, Entity_, CollidedWith);
                            }
                        }

                        if(dYaw)
                        {
                            entity *CollidedWith = 0;
                            collision Collision = {};
                            for(u32 VirtualEntityIndex = 0;
                                VirtualEntityIndex < VirtualEntities->Count;
                                ++VirtualEntityIndex)
                            {
                                v3 P = VirtualEntities->P[VirtualEntityIndex];
                                r32 OldYaw = Entity_->Yaw;
                                r32 NewYaw = OldYaw + dYaw;
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
                                    entity *OtherEntity_ = State->Entities + OtherEntityIndex;
                                    if(!CanCollide(State, Entity_, OtherEntity_) ||
                                       (OtherEntityIndex == EntityIndex)) continue;

                                    virtual_entities *OtherVirtualEntities = VirtualEntityTable + OtherEntityIndex;
                                    for(u32 OtherVirtualEntityIndex = 0;
                                        OtherVirtualEntityIndex < OtherVirtualEntities->Count;
                                        ++OtherVirtualEntityIndex)
                                    {
                                        v3 OtherP = OtherVirtualEntities->P[OtherVirtualEntityIndex];
                                        b32 BoundingBoxesOverlap;
                                        {
                                            r32 HitRadius = OtherEntity_->BoundingRadius + Entity_->BoundingRadius;

                                            BoundingBoxesOverlap = (HitRadius*HitRadius >=
                                                                    LengthSq(OtherP - P));
                                        }

                                        if(BoundingBoxesOverlap)
                                        {
#if COLLISION_DEBUG
                                            Entity->AngularBoundingCircleCollided[Entity->UsedAngularIterations] = true;
                                            OtherEntity->AngularBoundingCircleCollided[Entity->UsedAngularIterations] = true;
                                            u32 CollidingShapeMaskIndex = 1;
                                            u32 OtherCollidingShapeMaskIndex = 1;
#endif
                                            for(collision_shape *Shape = Entity_->CollisionShapes;
                                                Shape;
                                                Shape = Shape->Next)
                                            {
                                                for(collision_shape *OtherShape = OtherEntity_->CollisionShapes;
                                                    OtherShape;
                                                    OtherShape = OtherShape->Next)
                                                {

                                                    switch(Shape->Type|OtherShape->Type)
                                                    {
                                                        case CollisionShapePair_TriangleTriangle:
                                                        {
                                                            v2 Center = P.xy;
                                                            v2 A = P.xy + RotateZ(Shape->A, OldYaw);
                                                            v2 B = P.xy + RotateZ(Shape->B, OldYaw);
                                                            v2 C = P.xy + RotateZ(Shape->C, OldYaw);
                                                            v2 Centroid = 0.333f*(A + B + C);
                                                            r32 ARadius = Length(Shape->A);
                                                            r32 BRadius = Length(Shape->B);
                                                            r32 CRadius = Length(Shape->C);

                                                            v2 OtherA = OtherP.xy + RotateZ(OtherShape->A, OtherEntity_->Yaw);
                                                            v2 OtherB = OtherP.xy + RotateZ(OtherShape->B, OtherEntity_->Yaw);
                                                            v2 OtherC = OtherP.xy + RotateZ(OtherShape->C, OtherEntity_->Yaw);
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

                                                            r32 InittMove = tMove;
                                            
                                                            ProcessIntersection(IntersectionAAB, &tMove, &Collision);
                                                            ProcessIntersection(IntersectionABC, &tMove, &Collision);
                                                            ProcessIntersection(IntersectionACA, &tMove, &Collision);

                                                            ProcessIntersection(IntersectionBAB, &tMove, &Collision);
                                                            ProcessIntersection(IntersectionBBC, &tMove, &Collision);
                                                            ProcessIntersection(IntersectionBCA, &tMove, &Collision);

                                                            ProcessIntersection(IntersectionCAB, &tMove, &Collision);
                                                            ProcessIntersection(IntersectionCBC, &tMove, &Collision);
                                                            ProcessIntersection(IntersectionCCA, &tMove, &Collision);
                                            
                                                            ProcessIntersection(OtherIntersectionAAB, &tMove, &Collision);
                                                            ProcessIntersection(OtherIntersectionABC, &tMove, &Collision);
                                                            ProcessIntersection(OtherIntersectionACA, &tMove, &Collision);

                                                            ProcessIntersection(OtherIntersectionBAB, &tMove, &Collision);
                                                            ProcessIntersection(OtherIntersectionBBC, &tMove, &Collision);
                                                            ProcessIntersection(OtherIntersectionBCA, &tMove, &Collision);

                                                            ProcessIntersection(OtherIntersectionCAB, &tMove, &Collision);
                                                            ProcessIntersection(OtherIntersectionCBC, &tMove, &Collision);
                                                            ProcessIntersection(OtherIntersectionCCA, &tMove, &Collision);
                                            
                                                            if(InittMove != tMove)
                                                            {
                                                                CollidedWith = OtherEntity_;
#if COLLISION_DEBUG
                                                                CollidingShapeMask = CollidingShapeMaskIndex;
                                                                OtherCollidingShapeMask = OtherCollidingShapeMaskIndex;
#endif
                                                            }
                                                        } break;

                                                        case CollisionShapePair_CircleCircle:
                                                        {
                                                            r32 HitRadius = Shape->Radius + OtherShape->Radius;
                                                            v2 CenterOffset = RotateZ(Shape->Center, Entity_->Yaw);
                                                            v2 StartP = P.xy + CenterOffset;
                                                            r32 RotationRadius = Length(Shape->Center);
                                                            v2 A = OtherP.xy +
                                                                RotateZ(OtherShape->Center, OtherEntity_->Yaw);

                                                            arc_circle_intersection_result IntersectionA =
                                                                ArcCircleIntersection(P.xy, RotationRadius, StartP, dYaw, A, HitRadius);

                                                            r32 InittMove = tMove;
                                            
                                                            ProcessIntersection(IntersectionA, &tMove, &Collision);
                                            
                                                            if(InittMove != tMove)
                                                            {
                                                                CollidedWith = OtherEntity_;
#if COLLISION_DEBUG
                                                                CollidingShapeMask = CollidingShapeMaskIndex;
                                                                OtherCollidingShapeMask = OtherCollidingShapeMaskIndex;
#endif
                                                            }
                                                        } break;

                                                        case CollisionShapePair_TriangleCircle:
                                                        {
                                                            v3 TriangleP;
                                                            r32 TriangleYaw;
                                                            v3 CircleP;
                                                            collision_shape *CircleShape;
                                                            collision_shape *TriangleShape;
                                                            r32 RotationRadius;
                                                            v2 CenterOffset;
                                                            r32 AdjusteddYaw;
                        
                                                            if(Shape->Type == CollisionShapeType_Triangle)
                                                            {
                                                                TriangleP = P;
                                                                TriangleYaw = Entity_->Yaw;
                                                                TriangleShape = Shape;
                                                                CircleP = OtherP;
                                                                CircleShape = OtherShape;
                                                                CenterOffset = RotateZ(CircleShape->Center, OtherEntity_->Yaw);
                                                                RotationRadius = Length(CircleP.xy + CenterOffset - TriangleP.xy);
                                                                AdjusteddYaw = -dYaw;
                                                            }
                                                            else
                                                            {
                                                                TriangleP = OtherP;
                                                                TriangleYaw = OtherEntity_->Yaw;
                                                                TriangleShape = OtherShape;
                                                                CircleP = P;
                                                                CircleShape = Shape;
                                                                CenterOffset = RotateZ(CircleShape->Center, Entity_->Yaw);
                                                                RotationRadius = Length(CircleShape->Center);
                                                                AdjusteddYaw = dYaw;
                                                            }
                                                            r32 Radius = CircleShape->Radius;
                                                            v2 StartP = CircleP.xy + CenterOffset;
                                                            v2 A = TriangleP.xy + RotateZ(TriangleShape->A, TriangleYaw);
                                                            v2 B = TriangleP.xy + RotateZ(TriangleShape->B, TriangleYaw);
                                                            v2 C = TriangleP.xy + RotateZ(TriangleShape->C, TriangleYaw);
                        
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
                                                                ArcPolygonEdgeIntersection(P.xy, RotationRadius, StartP, AdjusteddYaw, ABHitA, ABHitB, C);
                                                            arc_polygon_edge_intersection_result IntersectionBC =
                                                                ArcPolygonEdgeIntersection(P.xy, RotationRadius, StartP, AdjusteddYaw, BCHitB, BCHitC, A);
                                                            arc_polygon_edge_intersection_result IntersectionCA =
                                                                ArcPolygonEdgeIntersection(P.xy, RotationRadius, StartP, AdjusteddYaw, CAHitC, CAHitA, B);

                                                            arc_circle_intersection_result IntersectionA =
                                                                ArcCircleIntersection(P.xy, RotationRadius, StartP, AdjusteddYaw, A, Radius);
                                                            arc_circle_intersection_result IntersectionB =
                                                                ArcCircleIntersection(P.xy, RotationRadius, StartP, AdjusteddYaw, B, Radius);
                                                            arc_circle_intersection_result IntersectionC =
                                                                ArcCircleIntersection(P.xy, RotationRadius, StartP, AdjusteddYaw, C, Radius);

                                                            r32 InittMove = tMove;
                                            
                                                            ProcessIntersection(IntersectionAB, &tMove, &Collision);
                                                            ProcessIntersection(IntersectionBC, &tMove, &Collision);
                                                            ProcessIntersection(IntersectionCA, &tMove, &Collision);

                                                            ProcessIntersection(IntersectionA, &tMove, &Collision);
                                                            ProcessIntersection(IntersectionB, &tMove, &Collision);
                                                            ProcessIntersection(IntersectionC, &tMove, &Collision);

                                                            if(InittMove != tMove)
                                                            {
                                                                CollidedWith = OtherEntity_;
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
                                }
                            }
                            Entity_->Yaw += dYaw*tMove;
                            Entity_->dYaw += Entity_->ddYaw*dtPhysics*tMove;
                            // TODO(chris): This is to clear out player input. How to keep around other forces?
                            Entity_->ddYaw = 0.0f;

                            if(CollidedWith)
                            {
#if COLLISION_DEBUG
                                Entity->AngularCollidingShapeMask[Entity->UsedAngularIterations] |= CollidingShapeMask;
                                CollidedWith->AngularCollidingShapeMask[CollidedWith->UsedAngularIterations] |= OtherCollidingShapeMask;
#endif
                                ResolveAngularCollision(&Collision, Entity_, CollidedWith);
                            }
                        }
#if COLLISION_DEBUG
                        Entity->CollisionStepP[CollisionIndex] = Entity->P;
                        Entity->CollisionStepYaw[CollisionIndex] = Entity->Yaw;
#endif
                        tMax -= tMove;
                    }
                }
            }
            END_TIMED_BLOCK(Collision);
        }
        
        // NOTE(chris): Post collision pass
        for(u32 EntityIndex = 1;
            EntityIndex < State->EntityCount;
            )
        {
            entity *Entity = State->Entities + EntityIndex;

            if(IsDestroyed(Entity))
            {
                switch(Entity->Type)
                {
                    case EntityType_Ship:
                    {
                        if(State->PlayType == PlayType_Arcade)
                        {
                            --State->Lives;
                        }
                        CreateShipExplosion(State, Entity, State->ShipColor);
                        if(State->Lives > 0)
                        {
                            CreateSpawnTimer(State);
                        }
                        else
                        {
                            GameOver(State, RenderBuffer, &GameMemory->Font);
                        }
                    } break;

                    case EntityType_EnemyShip:
                    {
                        State->EnemyState = EnemyState_NotHere;
                        if(State->PlayType == PlayType_Journey)
                        {
                            CreateMetamorphosisTimer(State);
                        }
                        CreateShipExplosion(State, Entity, State->EnemyColor);
                        if(Entity->DestroyedBy == ColliderType_Laser)
                        {
                            AddPoints(State, 1000);
                        }
                    } break;

                    case EntityType_Asteroid:
                    {
                        AddPoints(State, SplitAsteroid(State, Entity));
                    } break;

                    default:
                    {
                    } break;
                }
                if(IsDestroyed(Entity))
                {
                    DestroyEntity(State, Entity);
                }
                else
                {
                    ++EntityIndex;
                }
            }
            else
            {
                ++EntityIndex;
            }
        }
        
        if(State->SourcePoints != State->Points)
        {
            r32 PointsTimerMax = 1.0f;
            State->PointsTimer += Input->dtForFrame;
            if(State->PointsTimer >= PointsTimerMax)
            {
                State->SourcePoints = State->SpinningPoints = State->Points;
            }
            else
            {
                r32 tPoints = State->PointsTimer/PointsTimerMax;
                u32 Diff = State->Points - State->SourcePoints;
                u32 LerpedPoints = State->SourcePoints + (u32)(Diff*SquareRoot(tPoints));
                State->SpinningPoints = Clamp(State->SourcePoints,
                                              LerpedPoints,
                                              State->Points);
            }
        }
    }

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
#if COLLISION_FINE_DEBUG
        Entity->P = Entity->CollisionStepP[0];
        Entity->dP = Entity->InitdP;
        Entity->Yaw = Entity->CollisionStepYaw[0];
        Entity->dYaw = Entity->InitdYaw;
#endif
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
                DEBUGPushCircle(RenderBuffer, P + BoundingCircleOffset,
                                Entity->BoundingRadius, BoundingColor);
                
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
                            DEBUGPushCircle(RenderBuffer, P + RotatedCenter  + ShapeOffset,
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
                            DEBUGPushTriangle(RenderBuffer,
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
                    DEBUGPushRectangle(RenderBuffer, 0.5f*(OldP + P) + BoundingExtrusionOffset,
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
                                DEBUGPushRectangle(RenderBuffer,
                                                   RotatedOldCenter + 0.5f*(OldP + P) + ShapeExtrusionOffset,
                                                   XAxis, YAxis, ShapeDim, LinearShapeExtrusionColor);
                                DEBUGPushCircle(RenderBuffer, P + RotatedOldCenter + ShapeOffset,
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
                                DEBUGPushTriangle(RenderBuffer,
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

    BEGIN_TIMED_BLOCK(RenderPush, "Render Push");

    v3 LifeHUDP = {};
    v2 LifeHUDHalfDim = {};
    {
        RenderBuffer->Projection = Projection_None;
        text_layout Layout = {};
        Layout.Font = &GameMemory->Font;
        Layout.Scale = 0.5f;
        Layout.Color = V4(1, 1, 1, 1);

        if(State->Paused)
        {
            char PausedText[] = "PAUSED";
            u32 PausedTextLength = sizeof(PausedText) - 1;
            text_measurement PausedMeasurement = DrawText(0, &Layout,
                                                          PausedTextLength, PausedText,
                                                          DrawTextFlags_Measure);
            Layout.P = ScreenCenter + GetTightAlignmentOffset(PausedMeasurement);
            DrawText(RenderBuffer, &Layout, PausedTextLength, PausedText);
        }

        if(State->PlayType == PlayType_Arcade)
        {
            r32 HudMargin = 5.0f;
        
            v2 TopCenter = V2i(RenderBuffer->Width/2, RenderBuffer->Height) - V2(0.0f, HudMargin);
            char PointsText[16];
            u32 PointsTextLength = _snprintf_s(PointsText, sizeof(PointsText), "%010lu", State->SpinningPoints);
            text_measurement PointsMeasurement = DrawText(0, &Layout,
                                                          PointsTextLength, PointsText,
                                                          DrawTextFlags_Measure);
            Layout.P = TopCenter + GetTightAlignmentOffset(PointsMeasurement, TextAlignFlags_TopCenter);
            DrawText(RenderBuffer, &Layout, PointsTextLength, PointsText);

            v2 XAxis = V2(1, 0);
            v2 YAxis = Perp(XAxis);
            v2 Dim = (PointsMeasurement.TextMaxY - PointsMeasurement.TextMinY)*V2(1.0f, 1.0f);
            v2 HalfDim = 0.5f*Dim;
            v3 P = V3(V2i(RenderBuffer->Width, RenderBuffer->Height) - HalfDim - V2(HudMargin, HudMargin), 0);
            for(s32 LifeIndex = 1;
                LifeIndex < State->Lives;
                ++LifeIndex)
            {
                PushTriangle(RenderBuffer,
                             P + V3(0.97f*V2(0, HalfDim.y), HUD_Z),
                             P + V3(0.96f*V2(-HalfDim.x, -HalfDim.y), HUD_Z),
                             P + V3(0.69f*V2(0, -HalfDim.y), HUD_Z),
                             State->ShipColor);
                PushTriangle(RenderBuffer,
                             P + V3(0.97f*V2(0, HalfDim.y), HUD_Z),
                             P + V3(0.69f*V2(0, -HalfDim.y), HUD_Z),
                             P + V3(0.96f*V2(HalfDim.x, -HalfDim.y), HUD_Z),
                             State->ShipColor);
                P.x -= (Dim.x + HudMargin);
            }

            LifeHUDP = P;
            LifeHUDHalfDim = HalfDim;

        }
        RenderBuffer->Projection = Projection_Perspective;
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

            case EntityType_EnemySpawnTimer:
            case EntityType_Ship:
            case EntityType_EnemyShip:
            case EntityType_Asteroid:
            {
//                State->CameraP.xy += 0.2f*(Entity->P.xy - State->CameraP.xy);
//                State->CameraRot += 0.05f*((Entity->Yaw - 0.25f*Tau) - State->CameraRot);

                v4 Color = V4(1, 1, 1, 1);
                collision_shape *Shapes = Entity->CollisionShapes;
                if(Entity->Type == EntityType_Ship ||
                   Entity->Type == EntityType_EnemyShip ||
                   Entity->Type == EntityType_EnemySpawnTimer)
                {
                    entity *Ship = Entity;
                    if(Entity->Type == EntityType_EnemyShip)
                    {
                        Color = State->EnemyColor;
                    }
                    else if(Entity->Type == EntityType_EnemySpawnTimer)
                    {
                        Color = State->EnemyColor;
                        Color.a *= Clamp01((Entity->Duration-Entity->Timer) / Entity->Duration);
                        Ship = State->Entities + Entity->Target;
                        Shapes = Ship->CollisionShapes;
                    }
                    else if(Entity->Type == EntityType_Ship)
                    {
                        Color = State->ShipColor;
                    }
                    v2 EngineP = Ship->CollisionShapes[0].B;
                    v3 EngineA = V3(EngineP.x, EngineP.y - 1.0f, -0.001f);
                    v3 EngineB = V3(EngineP.x, EngineP.y + 1.0f, -0.001f);
                    v3 EngineC = V3(EngineP.x - 2.0f, EngineP.y, -0.001f);
                    v3 A = Entity->P + V3(RotateZ(EngineA.xy, Entity->Yaw), EngineA.z);
                    v3 B = Entity->P + V3(RotateZ(EngineB.xy, Entity->Yaw), EngineB.z);
                    v3 C = Entity->P + V3(RotateZ(EngineC.xy, Entity->Yaw), EngineC.z);
                    PushWraparoundTriangle(RenderBuffer, FieldRect, A, B, C, V4(1, 1, 1, Entity->Flicker));
                }

                for(collision_shape *Shape = Shapes;
                    Shape;
                    Shape = Shape->Next)
                {
                    v3 A = Entity->P + V3(RotateZ(Shape->A, Entity->Yaw), 0.0f);
                    v3 B = Entity->P + V3(RotateZ(Shape->B, Entity->Yaw), 0.0f);
                    v3 C = Entity->P + V3(RotateZ(Shape->C, Entity->Yaw), 0.0f);
                    
                    v3 Offset = V3(-FieldDim.x, 0, 0);
                    PushWraparoundTriangle(RenderBuffer, FieldRect, A, B, C, Color);
                }
            } break;

            case EntityType_SpawnTimer:
            {
                if(State->PlayType == PlayType_Arcade)
                {
                    RenderBuffer->Projection = Projection_None;
                    v3 P = LifeHUDP;
                    v2 HalfDim = LifeHUDHalfDim;
                    r32 Alpha = Clamp01(Entity->Timer / Entity->Duration);
                    PushTriangle(RenderBuffer,
                                 P + V3(0.97f*V2(0, HalfDim.y), HUD_Z),
                                 P + V3(0.96f*V2(-HalfDim.x, -HalfDim.y), HUD_Z),
                                 P + V3(0.69f*V2(0, -HalfDim.y), HUD_Z),
                                 V4(1, 1, 1, Alpha));
                    PushTriangle(RenderBuffer,
                                 P + V3(0.97f*V2(0, HalfDim.y), HUD_Z),
                                 P + V3(0.69f*V2(0, -HalfDim.y), HUD_Z),
                                 P + V3(0.96f*V2(HalfDim.x, -HalfDim.y), HUD_Z),
                                 V4(1, 1, 1, Alpha));
                    RenderBuffer->Projection = Projection_Perspective;
                }
            } break;

            case EntityType_Laser:
            case EntityType_EnemyLaser:
            {
                v3 OpaqueColor = State->ShipColor.rgb;
                if(Entity->Type == EntityType_EnemyLaser)
                {
                    OpaqueColor = State->EnemyColor.rgb;
                }
                v4 Color = V4(OpaqueColor, Entity->Timer/Entity->Duration);
                for(s32 Y = -1;
                    Y <= 1;
                    ++Y)
                {
                    for(s32 X = -1;
                        X <= 1;
                        ++X)
                    {
                        v3 Offset = V3(X*FieldDim.x, Y*FieldDim.y, 0);
                        PushBitmap(RenderBuffer, &GameState->LaserBitmap, Offset + Entity->P,
                                   XAxis, YAxis,
                                   Entity->Dim,
                                   Color);
                    }
                }
//            DrawLine(BackBuffer, State->P, Laser->P, V4(0.0f, 0.0f, 1.0f, 1.0f));
            } break;

            case EntityType_Letter:
            {
                XAxis = V2(1, 0);
                YAxis = Perp(XAxis);
                loaded_bitmap *Glyph = GameMemory->Font.Glyphs + Entity->Character;
                PushBitmap(RenderBuffer, Glyph, Entity->P,
                           XAxis, YAxis, Entity->Dim);
            } break;
        }
    }

    // NOTE(chris): Particle render pass
    for(u32 ParticleIndex = 1;
        ParticleIndex < State->ParticleCount;
        ++ParticleIndex)
    {
        particle *Particle = State->Particles + ParticleIndex;
        r32 Fade = Clamp01(Particle->Timer / Particle->Duration);
        v3 A = V3(Particle->P.xy + RotateZ(Particle->A, Particle->Yaw), Particle->P.z);
        v3 B = V3(Particle->P.xy + RotateZ(Particle->B, Particle->Yaw), Particle->P.z);
        v3 C = V3(Particle->P.xy + RotateZ(Particle->C, Particle->Yaw), Particle->P.z);
        PushWraparoundTriangle(RenderBuffer, FieldRect, A, B, C,
                               V4(Particle->Color.rgb, Particle->Color.a*Fade));
    }
    
    END_TIMED_BLOCK(RenderPush);

    if(State->Lives <= 0)
    {
        if(WentDown(ShipController->Start))
        {
            GameState->NextMode = GameMode_TitleScreen;
        }
    }
    else if(State->AsteroidCount <= 0 && State->EnemyState == EnemyState_NotHere)
    {
        if(State->ResetTimer <= 0.0f)
        {
            ResetField(State, RenderBuffer);
        }
        else
        {
            RenderBuffer->Projection = Projection_None;
            text_layout Layout = {};
            Layout.Font = &GameMemory->Font;
            Layout.Scale = 1.0f;
            Layout.Color = V4(1, 1, 1, 1);
            
            char Second = '0' + (char)Ceiling(State->ResetTimer);
            text_measurement SecondMeasurement = DrawText(0, &Layout,
                                                          1, &Second,
                                                          DrawTextFlags_Measure);
            Layout.P = ScreenCenter + GetTightAlignmentOffset(SecondMeasurement);
            DrawText(RenderBuffer, &Layout, 1, &Second);
            RenderBuffer->Projection = Projection_Perspective;

            State->ResetTimer -= Input->dtForFrame;
        }
    }
    else
    {
        if(WentDown(ShipController->Start) && State->ShipColor.rgb != V3(0, 0, 0))
        {
            State->Paused = !State->Paused;
        }
    }

    if(WentDown(ShipController->Select))
    {
        GameState->NextMode = GameMode_TitleScreen;
    }

    {
        TIMED_BLOCK("Render Game");
        RenderBufferToBackBuffer(RendererState, RenderBuffer, RenderFlags_UsePipeline);
    }
    
    {DEBUG_GROUP("Play Mode");
        DEBUG_FILL_BAR("Entities", State->EntityCount, ArrayCount(State->Entities));
        DEBUG_FILL_BAR("Particles", State->ParticleCount, ArrayCount(State->Particles));
        DEBUG_VALUE("Shape Arena", State->PhysicsState.ShapeArena);
        DEBUG_VALUE("Shape Freelist", (b32)State->PhysicsState.FirstFreeShape);
        DEBUG_VALUE("Render Arena", TranState->RenderBuffer.Arena);
    }
    EndTemporaryMemory(RenderMemory);
}
