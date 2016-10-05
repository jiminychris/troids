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
    Result->ColliderType = ColliderType_Ship;
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
CreateAsteroid(play_state *State, r32 MaxRadius, rectangle2 Cell)
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
    Result->P = V3(RandomBetween(Seed, -100.0f, 100.0f), RandomBetween(Seed, -100.0f, 100.0f), 0.0f);
    Result->P = V3(0.5f*(Cell.Min + Cell.Max), 0.0f);
    Result->dP = V3(RandomBetween(Seed, -20.0f, 20.0f), RandomBetween(Seed, -20.0f, 20.0f), 0.0f);
//    Result->dP = {};
    Result->Yaw = RandomBetween(Seed, 0.0f, Tau);
//    Result->Yaw = 0.0f;
    Result->dYaw = RandomBetween(Seed, -0.5f*Tau, 0.5f*Tau);
//    Result->dYaw = 0.0f;
    
    return(Result);
}

inline void
SplitAsteroid(play_state *State, entity *Asteroid)
{
    if(!Asteroid->Disintegrated && Asteroid->CollisionShapeCount > 3)
    {
        u32 FirstSplitShapeIndex = RandomBetween(&State->AsteroidSeed,
                                                 2, Asteroid->CollisionShapeCount-2);
        Asteroid->Destroyed = false;
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
        State->AsteroidCount += 1;
    }
    else
    {
        State->AsteroidCount -= 1;
    }
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
CreateLaser(play_state *State, v3 P, v3 dP, r32 Yaw)
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
ResetGame(play_state *State, render_buffer *RenderBuffer, loaded_font *Font)
{
    RenderBuffer->CameraRot = 0.0f;

    // NOTE(chris): Null entity
    State->EntityCount = 1;
    State->PhysicsState.ShapeArena.Used = 0;
    State->PhysicsState.FirstFreeShape = 0;

    State->ShipStartingP = V3(0.0f, 0.0f, 0.0f);
    State->CameraStartingP = V3(0, 0, 0);
    State->ShipStartingYaw = 0.0f;
    entity Ship_;
    Ship_.BoundingRadius = 0.0f;
    Ship_.P = V3(0.0f, 0.0f, 0.0f);
    entity *Ship = &Ship_;
    if(State->Lives)
    {
        Ship = CreateShip(State, State->ShipStartingP,
                          State->ShipStartingYaw);
    }
    else
    {
        GameOver(State, RenderBuffer, Font);
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

    Assert(CellDim.x*GridDimInCells.x == GridDimInMeters.x);
    Assert(CellDim.y*GridDimInCells.y == GridDimInMeters.y);

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
}

internal void
PlayMode(game_memory *GameMemory, game_input *Input, renderer_state *RendererState)
{
    game_state *GameState = (game_state *)GameMemory->PermanentMemory;
    transient_state *TranState = (transient_state *)GameMemory->TemporaryMemory;
    play_state *State = &GameState->PlayState;

    render_buffer *RenderBuffer = &TranState->RenderBuffer;
    RenderBuffer->Projection = RenderBuffer->DefaultProjection;
    
    if(!State->IsInitialized)
    {
        State->IsInitialized = true;

        State->AsteroidSeed = Seed(Input->SeedValue);

        InitializePhysicsState(&State->PhysicsState, &GameState->Arena);

        AllowCollision(&State->PhysicsState, ColliderType_Ship, ColliderType_Asteroid);
        AllowCollision(&State->PhysicsState, ColliderType_Asteroid, ColliderType_Asteroid);
        AllowCollision(&State->PhysicsState, ColliderType_Ship, ColliderType_Wall);
        AllowCollision(&State->PhysicsState, ColliderType_Asteroid, ColliderType_Wall);
        AllowCollision(&State->PhysicsState, ColliderType_Asteroid, ColliderType_Laser);
        AllowCollision(&State->PhysicsState, ColliderType_Asteroid, ColliderType_IndestructibleLaser);

        State->Lives = 3;
        State->Difficulty = 4;
        ResetGame(State, RenderBuffer, &GameMemory->Font);
    }
    
    {DEBUG_GROUP("Play Mode");
        DEBUG_FILL_BAR("Entities", State->EntityCount, ArrayCount(State->Entities));
        DEBUG_VALUE("Shape Arena", State->PhysicsState.ShapeArena);
        DEBUG_VALUE("Shape Freelist", (b32)State->PhysicsState.FirstFreeShape);
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

    game_controller *Keyboard = &Input->Keyboard;
    game_controller *ShipController = Input->GamePads + 0;
    game_controller *AsteroidController = Input->GamePads + 1;
        
    rectangle2 FieldRect = MinMax(Unproject(RenderBuffer, V3(0, 0, 0)).xy,
                                  Unproject(RenderBuffer,
                                            V3i(RenderBuffer->Width, RenderBuffer->Height, 0)).xy);
    v2 FieldDim = GetDim(FieldRect);
    v3 FieldWidthOffset = V3(FieldDim.x, 0, 0);
    v3 FieldHeightOffset = V3(0, FieldDim.y, 0);

    b32 FirstAsteroid = true;
    // NOTE(chris): Pre collision pass
    for(u32 EntityIndex = 1;
        EntityIndex < State->EntityCount;
        )
    {
        u32 NextIndex = EntityIndex + 1;
        entity *Entity = State->Entities + EntityIndex;

        if((Entity->P.x - Entity->BoundingRadius) > FieldRect.Max.x)
        {
            Entity->P.x -= FieldDim.x;
        }
        if((Entity->P.x + Entity->BoundingRadius) < FieldRect.Min.x)
        {
            Entity->P.x += FieldDim.x;
        }
        if((Entity->P.y - Entity->BoundingRadius) > FieldRect.Max.y)
        {
            Entity->P.y -= FieldDim.y;
        }
        if((Entity->P.y + Entity->BoundingRadius) < FieldRect.Min.y)
        {
            Entity->P.y += FieldDim.y;
        }

        v3 Facing = V3(Cos(Entity->Yaw), Sin(Entity->Yaw), 0.0f);
        switch(Entity->Type)
        {
            case EntityType_Ship:
            {
#if TROIDS_INTERNAL
                if(Keyboard->Start.EndedDown || ShipController->Start.EndedDown)
                {
                    Entity->P = State->ShipStartingP;
                }
#endif

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
                r32 ddYaw = -2.0f*LeftStickX;
#endif
                r32 dYaw = ddYaw*Input->dtForFrame;
                r32 MaxdYawPerFrame = 0.49f*Tau;
                r32 MaxdYawPerSecond = MaxdYawPerFrame/Input->dtForFrame;
                Entity->dYaw = Clamp(-MaxdYawPerSecond, Entity->dYaw + dYaw, MaxdYawPerSecond);
    
                v3 Acceleration = {};
                if(ShipController->ActionDown.EndedDown || Keyboard->ActionDown.EndedDown)
                {
                    if(Entity->Timer <= 0.0f)
                    {
                        entity *Laser = CreateLaser(State, Entity->P, Entity->dP + Facing*100.0f,
                                                      Entity->Yaw);
                        r32 LaserOffset = 0.5f*(Entity->Dim.y + Laser->Dim.y);
                        Laser->P += Facing*(LaserOffset);
                        Entity->Timer = 0.1f;
                        Acceleration += -Facing*100.0f;
                    }
                }
#if TROIDS_INTERNAL
                if(WentDown(ShipController->ActionRight) || WentDown(Keyboard->ActionRight))
                {
                    Entity->Destroyed = true;
                }
                Acceleration += V3(1000.0f*ShipController->RightStick*Input->dtForFrame, 0);
#endif

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

            case EntityType_Laser:
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

            case EntityType_Letter:
            {
                if(Entity->Timer == 0.0f)
                {
                    Entity->Timer += Input->dtForFrame;
                }
                else
                {
                    Entity->ColliderType = ColliderType_Wall;
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
        
        EntityIndex = NextIndex;
        Entity->Yaw = RealMod(Entity->Yaw, Tau);
        Assert(0 <= Entity->Yaw && Entity->Yaw <= Tau);
    }

    BEGIN_TIMED_BLOCK(Collision, "Collision");
    // NOTE(chris): Collision pass
    for(u32 EntityIndex = 1;
        EntityIndex < State->EntityCount;
        ++EntityIndex)
    {
        entity *Entity = State->Entities + EntityIndex;
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

                                            r32 InittMove = tMove;
                                            
                                            ProcessIntersection(IntersectionA, &tMove, &Collision);
                                            
                                            if(InittMove != tMove)
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

                                            r32 InittMove = tMove;
                                            
                                            ProcessIntersection(IntersectionAB, &tMove, &Collision);
                                            ProcessIntersection(IntersectionBC, &tMove, &Collision);
                                            ProcessIntersection(IntersectionCA, &tMove, &Collision);

                                            ProcessIntersection(IntersectionA, &tMove, &Collision);
                                            ProcessIntersection(IntersectionB, &tMove, &Collision);
                                            ProcessIntersection(IntersectionC, &tMove, &Collision);

                                            if(InittMove != tMove)
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
                        ResolveAngularCollision(&Collision, Entity, CollidedWith);
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
                        Entity->P = State->ShipStartingP;
                        Entity->dP = {};
                        Entity->Yaw = State->ShipStartingYaw;
                        Entity->dYaw = 0.0f;
                        Entity->Timer = 0;
                        Entity->Destroyed = false;
                    }
                    else
                    {
                        GameOver(State, RenderBuffer, &GameMemory->Font);
                    }
                } break;

                case EntityType_Asteroid:
                {
                    SplitAsteroid(State, Entity);
                } break;

                default:
                {
                } break;
            }
            if(Entity->Destroyed)
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

    BEGIN_TIMED_BLOCK(RenderPush, "Render Push");
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
            case EntityType_Asteroid:
            {
//                State->CameraP.xy += 0.2f*(Entity->P.xy - State->CameraP.xy);
//                State->CameraRot += 0.05f*((Entity->Yaw - 0.25f*Tau) - State->CameraRot);
//                PushBitmap(RenderBuffer, &GameState->ShipBitmap, Entity->P,
//                           XAxis, YAxis, Entity->Dim);
                v4 Color = V4(1, 1, 1, 1);
                for(collision_shape *Shape = Entity->CollisionShapes;
                    Shape;
                    Shape = Shape->Next)
                {
                    v3 A = Entity->P + V3(RotateZ(Shape->A, Entity->Yaw), 0.0f);
                    v3 B = Entity->P + V3(RotateZ(Shape->B, Entity->Yaw), 0.0f);
                    v3 C = Entity->P + V3(RotateZ(Shape->C, Entity->Yaw), 0.0f);
                    
                    v3 Offset = V3(-FieldDim.x, 0, 0);
                    PushTriangle(RenderBuffer,
                                 A,
                                 B,
                                 C,
                                 Color);
                    b32 WrapRight = (Entity->P.x + Entity->BoundingRadius) > FieldRect.Max.x;
                    b32 WrapLeft = (Entity->P.x - Entity->BoundingRadius) < FieldRect.Min.x;
                    b32 WrapTop = (Entity->P.y + Entity->BoundingRadius) > FieldRect.Max.y;
                    b32 WrapBottom = (Entity->P.y - Entity->BoundingRadius) < FieldRect.Min.y;
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
            } break;

            case EntityType_Laser:
            {
                v4 Color = V4(1.0f, 1.0f, 1.0f, Unlerp(0.0f, Entity->Timer, 2.0f));
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
        }
    }
    END_TIMED_BLOCK(RenderPush);

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
            PushTriangle(RenderBuffer,
                         P + 0.97f*V3(0, HalfDim.y, 0),
                         P + 0.96f*V3(-HalfDim.x, -HalfDim.y, 0),
                         P + 0.69f*V3(0, -HalfDim.y, 0),
                         V4(1, 1, 1, 1));
            PushTriangle(RenderBuffer,
                         P + 0.97f*V3(0, HalfDim.y, 0),
                         P + 0.69f*V3(0, -HalfDim.y, 0),
                         P + 0.96f*V3(HalfDim.x, -HalfDim.y, 0),
                         V4(1, 1, 1, 1));
            P.x -= 1.2f*Dim.x;
        }
        RenderBuffer->Projection = Projection_Perspective;
    }

    if(State->Lives <= 0)
    {
        if(WentDown(ShipController->Start))
        {
            State->Lives = 3;
            State->Difficulty = 4;
            ResetGame(State, RenderBuffer, &GameMemory->Font);
        }
    }
    if(State->AsteroidCount <= 0)
    {
        State->Difficulty *= 2;
        ResetGame(State, RenderBuffer, &GameMemory->Font);
    }
    if(WentDown(ShipController->Select))
    {
        GameState->NextMode = GameMode_TitleScreen;
    }

    {
        TIMED_BLOCK("Render Game");
        RenderBufferToBackBuffer(RendererState, RenderBuffer, RenderFlags_UsePipeline);
    }
    EndTemporaryMemory(RenderMemory);
}
