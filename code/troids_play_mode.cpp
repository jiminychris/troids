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
    collision_shape Shapes[] =
    {
        CollisionCircle(Radius),
    };

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
    // TODO(chris): Real shapes in here!
    collision_shape Shapes[] =
    {
        CollisionTriangle(V2(0.0f*Dim.x, 0.0f*Dim.y),
                          V2(0.58f*Dim.x, Dim.y),
                          V2(0.423f*Dim.x, Dim.y)),
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
    // TODO(chris): Real shapes in here!
    collision_shape Shapes[] =
    {
        CollisionTriangle(V2(0.0f*Dim.x, 0.0f*Dim.y),
                          V2(0.58f*Dim.x, Dim.y),
                          V2(0.423f*Dim.x, Dim.y)),
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
    
    State->EntityCount = 0;
    State->PhysicsState.ShapeArena.Used = 0;

#if 0
    State->Lives = 3;
#else
    State->Lives = 0;
#endif

    State->ShipStartingP = V3(0, 0, 0);
    State->ShipStartingYaw = 0;
    if(State->Lives)
    {
        entity *Ship = CreateShip(State, State->ShipStartingP, State->ShipStartingYaw);
    }
    else
    {
        GameOver(State, RenderBuffer, Font);
    }
    entity *FloatingHead = CreateFloatingHead(State);

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
    }
    
    temporary_memory RenderMemory = BeginTemporaryMemory(&RenderBuffer->Arena);
    PushClear(RenderBuffer, V4(0.0f, 0.0f, 0.0f, 1.0f));
    v2 ScreenCenter = 0.5f*V2i(RenderBuffer->Width, RenderBuffer->Height); 

    game_controller *Keyboard = &Input->Keyboard;
    game_controller *ShipController = Input->GamePads + 0;
    game_controller *AsteroidController = Input->GamePads + 1;

    b32 FirstAsteroid = true;
    // NOTE(chris): Pre collision pass
    for(u32 EntityIndex = 0;
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
    
                r32 MaxDYaw = 0.2*Tau;
                Entity->dYaw = Clamp(-MaxDYaw, Entity->dYaw + Input->dtForFrame*-LeftStickX, MaxDYaw);
    
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

                Acceleration += 100.0f*Facing*Thrust;
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
            Entity->BoundingCircleCollided = false;
            for(collision_shape *Shape = Entity->CollisionShapes;
                Shape;
                Shape = Shape->Next)
            {
                Shape->Collided = false;
            }
#endif
        }
        EntityIndex = NextIndex;
    }

    BEGIN_TIMED_BLOCK("Collision");
    // NOTE(chris): Collision pass
    for(u32 EntityIndex = 0;
        EntityIndex < State->EntityCount;
        ++EntityIndex)
    {
        entity *Entity = State->Entities + EntityIndex;
        r32 tMax = 1.0f;
        if(CanCollide(Entity))
        {
            for(u32 CollisionIndex = 0;
                CollisionIndex < COLLISION_ITERATIONS;
                ++CollisionIndex)
            {
                entity *CollidedWith = 0;
                collision_shape *CollidingShape = 0;
                collision_shape *OtherCollidingShape = 0;
                collision Collision = {};
                v3 OldP = Entity->P;
                v3 NewP = OldP + Input->dtForFrame*Entity->dP;
                v3 dP = NewP - OldP;
                r32 tMove = tMax;
                if(dP.x != 0 || dP.y != 0)
                {
                    r32 InvdPY = 1.0f / dP.y;
                    for(u32 OtherEntityIndex = 0;
                        OtherEntityIndex < State->EntityCount;
                        ++OtherEntityIndex)
                    {
                        entity *OtherEntity = State->Entities + OtherEntityIndex;
                        if(!CanCollide(State, Entity, OtherEntity) ||
                           (OtherEntityIndex == EntityIndex)) continue;

                        if(BoundingCirclesIntersect(Entity, OtherEntity, NewP))
                        {
#if COLLISION_DEBUG_LINEAR
                            Entity->BoundingCircleCollided = true;
                            OtherEntity->BoundingCircleCollided = true;
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
                                            NotImplemented;
                                        } break;

                                        case CollisionShapePair_CircleCircle:
                                        {
                                            v2 Start = Entity->P.xy + RotateZ(Shape->Center, Entity->Yaw);
                                            v2 End = Start + Input->dtForFrame*Entity->dP.xy;
                                            v2 dMove = End - Start;
                                            r32 HitRadius = Shape->Radius + OtherShape->Radius;
                                            v2 HitCenter = (OtherEntity->P.xy +
                                                            RotateZ(OtherShape->Center, OtherEntity->Yaw));

                                            circle_ray_intersection_result Intersection =
                                                                             CircleRayIntersection(HitCenter, HitRadius, Start, End);

                                            if(ProcessIntersection(Intersection, &tMove, tMax, dMove))
                                            {
                                                Collision.Type = CollisionType_Circle;
                                                Collision.Deflection = Start - HitCenter;

                                                CollidedWith = OtherEntity;
                                                CollidingShape = Shape;
                                                OtherCollidingShape = OtherShape;
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
                                                End = Start - Input->dtForFrame*Entity->dP.xy;
                                                Radius = OtherShape->Radius;
                                                A = Entity->P.xy + RotateZ(Shape->A, Entity->Yaw);
                                                B = Entity->P.xy + RotateZ(Shape->B, Entity->Yaw);
                                                C = Entity->P.xy + RotateZ(Shape->C, Entity->Yaw);
                                            }
                                            else
                                            {
                                                Start = Entity->P.xy + RotateZ(Shape->Center, Entity->Yaw);
                                                End = Start + Input->dtForFrame*Entity->dP.xy;
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

                                            r32 IntersectionAB = TriangleEdgeRayIntersection(ABHitA, ABHitB, Start, End, C);
                                            r32 IntersectionBC = TriangleEdgeRayIntersection(BCHitB, BCHitC, Start, End, A);
                                            r32 IntersectionCA = TriangleEdgeRayIntersection(CAHitC, CAHitA, Start, End, B);

                                            b32 IntersectionAUpdated = ProcessIntersection(IntersectionA, &tMove, tMax, dMove);
                                            b32 IntersectionBUpdated = ProcessIntersection(IntersectionB, &tMove, tMax, dMove);
                                            b32 IntersectionCUpdated = ProcessIntersection(IntersectionC, &tMove, tMax, dMove);
                                            b32 IntersectionABUpdated = ProcessIntersection(IntersectionAB, &tMove, tMax, dMove);
                                            b32 IntersectionBCUpdated = ProcessIntersection(IntersectionBC, &tMove, tMax, dMove);
                                            b32 IntersectionCAUpdated = ProcessIntersection(IntersectionCA, &tMove, tMax, dMove);

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
                                                CollidingShape = Shape;
                                                OtherCollidingShape = OtherShape;
                                            }
                                        } break;

                                        default:
                                        {
                                            NotImplemented;
                                        } break;
                                    }
                                }
                            }
                        }
                    }
#if 0
                    if(tMove != tMax)
                    {
                        v3 dP = Entity->dP*Input->dtForFrame*tMove;
                        r32 Root = LengthSq(dP)-COLLISION_EPSILON;
                        r32 Len = Length(dP);
                        if(Root >= 0 && Len >= 0)
                        {
                            tMove *= SquareRoot(Root)/Len;
                        }
                    }
#endif
                    Entity->P += Entity->dP*Input->dtForFrame*tMove;
                    tMax -= tMove;
                    
                    if(CollidedWith)
                    {
#if COLLISION_DEBUG_LINEAR
                        CollidingShape->Collided = true;
                        OtherCollidingShape->Collided = true;
#endif
                        ResolveLinearCollision(&Collision, Entity, CollidedWith);
                    }
                }

                CollidedWith = 0;
                Collision.Type = CollisionType_None;
                r32 OldYaw = Entity->Yaw;
                r32 NewYaw = OldYaw + Input->dtForFrame*Entity->dYaw;
                r32 dYaw = NewYaw - OldYaw;
                if(dYaw)
                {
                    r32 tYawMax = tMove;
                    for(u32 OtherEntityIndex = 0;
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
#if COLLISION_DEBUG_ANGULAR
                            Entity->BoundingCircleCollided = true;
                            OtherEntity->BoundingCircleCollided = true;
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
                                            NotImplemented;
                                        } break;

                                        case CollisionShapePair_CircleCircle:
                                        {
                                            r32 HitRadius = Shape->Radius + OtherShape->Radius;
                                            v2 CenterOffset = RotateZ(Shape->Center, Entity->Yaw);
                                            v2 StartP = Entity->P.xy + CenterOffset;
                                            r32 dYaw = NewYaw-OldYaw;
                                            r32 RotationRadius = Length(Shape->Center);
                                            v2 A = OtherEntity->P.xy +
                                                RotateZ(OtherShape->Center, OtherEntity->Yaw);

                                            arc_circle_intersection_result IntersectionA =
                                                ArcCircleIntersection(Entity->P.xy, RotationRadius, StartP, dYaw, A, HitRadius);

                                            if(ProcessIntersection(IntersectionA, RotationRadius, &tMove, tYawMax, dYaw))
                                            {
                                                CollidedWith = OtherEntity;
                                                CollidingShape = Shape;
                                                OtherCollidingShape = OtherShape;
                                            }
                                        } break;

                                        case CollisionShapePair_TriangleCircle:
                                        {
                                            r32 dYaw;
                                            entity *CircleEntity;
                                            entity *TriangleEntity;
                                            collision_shape *CircleShape;
                                            collision_shape *TriangleShape;
                                            r32 RotationRadius;
                                            v2 CenterOffset;
                        
                                            if(Shape->Type == CollisionShapeType_Triangle)
                                            {
                                                TriangleEntity = Entity;
                                                TriangleShape = Shape;
                                                CircleEntity = OtherEntity;
                                                CircleShape = OtherShape;
                                                dYaw = OldYaw-NewYaw;
                                                CenterOffset = RotateZ(CircleShape->Center, CircleEntity->Yaw);
                                                RotationRadius = Length(CircleEntity->P.xy + CenterOffset - TriangleEntity->P.xy);
                                            }
                                            else
                                            {
                                                TriangleEntity = OtherEntity;
                                                TriangleShape = OtherShape;
                                                CircleEntity = Entity;
                                                CircleShape = Shape;
                                                dYaw = NewYaw-OldYaw;
                                                CenterOffset = RotateZ(CircleShape->Center, CircleEntity->Yaw);
                                                RotationRadius = Length(CircleShape->Center);
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

                                            arc_triangle_edge_intersection_result IntersectionAB =
                                                ArcTriangleEdgeIntersection(Entity->P.xy, RotationRadius, StartP, dYaw, ABHitA, ABHitB, C);
                                            arc_triangle_edge_intersection_result IntersectionBC =
                                                ArcTriangleEdgeIntersection(Entity->P.xy, RotationRadius, StartP, dYaw, BCHitB, BCHitC, A);
                                            arc_triangle_edge_intersection_result IntersectionCA =
                                                ArcTriangleEdgeIntersection(Entity->P.xy, RotationRadius, StartP, dYaw, CAHitC, CAHitA, B);

                                            arc_circle_intersection_result IntersectionA =
                                                ArcCircleIntersection(Entity->P.xy, RotationRadius, StartP, dYaw, A, Radius);
                                            arc_circle_intersection_result IntersectionB =
                                                ArcCircleIntersection(Entity->P.xy, RotationRadius, StartP, dYaw, B, Radius);
                                            arc_circle_intersection_result IntersectionC =
                                                ArcCircleIntersection(Entity->P.xy, RotationRadius, StartP, dYaw, C, Radius);

                                            b32 IntersectionABUpdated = ProcessIntersection(IntersectionAB, RotationRadius, &tMove, tYawMax, dYaw);
                                            b32 IntersectionBCUpdated = ProcessIntersection(IntersectionBC, RotationRadius, &tMove, tYawMax, dYaw);
                                            b32 IntersectionCAUpdated = ProcessIntersection(IntersectionCA, RotationRadius, &tMove, tYawMax, dYaw);

                                            b32 IntersectionAUpdated = ProcessIntersection(IntersectionA, RotationRadius, &tMove, tYawMax, dYaw);
                                            b32 IntersectionBUpdated = ProcessIntersection(IntersectionB, RotationRadius, &tMove, tYawMax, dYaw);
                                            b32 IntersectionCUpdated = ProcessIntersection(IntersectionC, RotationRadius, &tMove, tYawMax, dYaw);

                                            if(IntersectionABUpdated || IntersectionBCUpdated || IntersectionCAUpdated ||
                                               IntersectionAUpdated || IntersectionBUpdated || IntersectionCUpdated)
                                            {
                                                CollidedWith = OtherEntity;
                                                CollidingShape = Shape;
                                                OtherCollidingShape = OtherShape;
                                            }
                                        } break;

                                        default:
                                        {
                                            NotImplemented;
                                        } break;
                                    }
                                }
                            }
                        }
                    }
                    Entity->Yaw += Entity->dYaw*Input->dtForFrame*tMove;

                    if(CollidedWith)
                    {
#if COLLISION_DEBUG_ANGULAR
                        CollidingShape->Collided = true;
                        OtherCollidingShape->Collided = true;
#endif
                        ResolveAngularCollision(Entity, CollidedWith);
                    }
                }

                {
#if COLLISION_DEBUG
#define BOUNDING_EXTRUSION_Z_OFFSET -0.0004f
#define BOUNDING_Z_OFFSET -0.0002f
#define SHAPE_EXTRUSION_Z_OFFSET 0.0002f
#define SHAPE_Z_OFFSET 0.0004f
#if 0
                    PushLine(RenderBuffer,
                             Entity->P + V3(0, 0, 0.1f),
                             Entity->P + Entity->dP + V3(0, 0, 0.1f),
                             V4(0, 0, 1, 0));
#endif
                    v3 BoundingExtrusionOffset = V3(0, 0, BOUNDING_EXTRUSION_Z_OFFSET);
                    v3 BoundingCircleOffset = V3(0, 0, BOUNDING_Z_OFFSET);
                    v3 ShapeExtrusionOffset = V3(0, 0, SHAPE_EXTRUSION_Z_OFFSET);
                    v3 ShapeOffset = V3(0, 0, SHAPE_Z_OFFSET);
            
                    v4 BoundingColor = Entity->BoundingCircleCollided ? V4(1, 0, 0, 1) : V4(1, 0, 1, 1);
                    v4 BoundingExtrusionColor = Entity->BoundingCircleCollided ? V4(0.8f, 0, 0, 1) : V4(0.8f, 0, 0.8f, 1);
                    PushCircle(RenderBuffer, OldP + BoundingCircleOffset, Entity->BoundingRadius, BoundingColor);
                    PushCircle(RenderBuffer, NewP + BoundingCircleOffset, Entity->BoundingRadius, BoundingColor);

                    r32 dPLength = Length(dP.xy);
                    r32 InvdPLength = 1.0f / dPLength;
                    v2 XAxis = InvdPLength*-Perp(dP.xy);
                    v2 YAxis = Perp(XAxis);

                    v2 BoundingDim = V2(2.0f*Entity->BoundingRadius, dPLength);
                    PushRotatedRectangle(RenderBuffer, 0.5f*(OldP + NewP) + BoundingExtrusionOffset,
                                         XAxis, YAxis, BoundingDim, BoundingExtrusionColor);
#if 1
                    for(collision_shape *Shape = Entity->CollisionShapes;
                        Shape;
                        Shape = Shape->Next)
                    {
                        v4 ShapeColor = Shape->Collided ? V4(0, 1, 0, 1) : V4(0, 1, 1, 1);
                        v4 ShapeExtrusionColor = Shape->Collided ? V4(0, 0.8f, 0, 1) : V4(0, 0.8f, 0.8f, 1);
                        switch(Shape->Type)
                        {
                            case CollisionShapeType_Circle:
                            {
                                v3 Center = V3(Shape->Center, 0);
                                v3 RotatedOldCenter = RotateZ(Center, OldYaw);
                                v3 RotatedNewCenter = RotateZ(Center, NewYaw);
                                PushCircle(RenderBuffer, OldP + RotatedOldCenter  + ShapeOffset,
                                           Shape->Radius, ShapeColor);
                                PushCircle(RenderBuffer, NewP + RotatedNewCenter + ShapeOffset,
                                           Shape->Radius, ShapeColor);
                                v2 ShapeDim = V2(2.0f*Shape->Radius, dPLength);
                                PushRotatedRectangle(RenderBuffer,
                                                     RotatedOldCenter + 0.5f*(OldP + NewP) + ShapeExtrusionOffset,
                                                     XAxis, YAxis, ShapeDim, ShapeExtrusionColor);
                            } break;

                            case CollisionShapeType_Triangle:
                            {
                                v3 A = V3(Shape->A, 0);
                                v3 B = V3(Shape->B, 0);
                                v3 C = V3(Shape->C, 0);
                                A = RotateZ(A, Entity->Yaw);
                                B = RotateZ(B, Entity->Yaw);
                                C = RotateZ(C, Entity->Yaw);
                                PushTriangle(RenderBuffer,
                                             OldP + A + ShapeOffset,
                                             OldP + B + ShapeOffset,
                                             OldP + C + ShapeOffset,
                                             ShapeColor);
                                PushTriangle(RenderBuffer,
                                             NewP + A + ShapeOffset,
                                             NewP + B + ShapeOffset,
                                             NewP + C + ShapeOffset,
                                             ShapeColor);
                            } break;
                        }
                    }
#endif
#endif
                }


                if(tMax <= 0.0f)
                {
                    break;
                }
            }
        }
    }
    END_TIMED_BLOCK();

    if(Keyboard->ActionUp.EndedDown || ShipController->ActionUp.EndedDown)
    {
        RenderBuffer->CameraP.z += 100.0f*Input->dtForFrame;
    }

    if(Keyboard->ActionDown.EndedDown || ShipController->ActionDown.EndedDown)
    {
        RenderBuffer->CameraP.z -= 100.0f*Input->dtForFrame;
    }

    // NOTE(chris): Post collision pass
    for(u32 EntityIndex = 0;
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
                    if(State->Lives)
                    {
                        CreateShip(State, State->ShipStartingP, State->ShipStartingYaw);
                    }
                    else
                    {
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

    if(State->DEBUGEntity)
    {
        DestroyEntity(State, State->DEBUGEntity);
        State->DEBUGEntity = 0;
    }
    if(!State->Lives)
    {
        text_layout Layout = {};
        Layout.Font = &GameMemory->Font;
        Layout.Scale = 0.6f;
        Layout.Color = V4(1, 1, 1, 1);
        
        if(State->Lives <= 0)
        {
            Layout.Scale = 1.0f;
            char GameOverText[] = "GAME OVER";
            text_measurement GameOverMeasurement = DrawText(RenderBuffer, &Layout,
                                                            sizeof(GameOverText)-1, GameOverText,
                                                            DrawTextFlags_Measure);
            Layout.P = ScreenCenter + GetTightCenteredOffset(GameOverMeasurement);
#if 0
            RenderBuffer->Projection = Projection_None;
            DrawText(RenderBuffer, &Layout, sizeof(GameOverText)-1, GameOverText);
            RenderBuffer->Projection = RenderBuffer->DefaultProjection;
            Layout.P = ScreenCenter + GetTightCenteredOffset(GameOverMeasurement);
#endif
            v3 P = V3(0.0f, -65.0f, 0.0f);
            r32 Scale = 0.6f;
//            State->DEBUGEntity = CreateR(State, &GameMemory->Font, P, Scale);
        }

        if(WentDown(ShipController->Start))
        {
            ResetGame(State, RenderBuffer, &GameMemory->Font);
        }

    }
    
    // NOTE(chris): Render pass
    for(u32 EntityIndex = 0;
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
