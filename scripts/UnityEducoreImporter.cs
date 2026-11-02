// Unity AssetPostprocessor & Runtime Metadata Components for Educore glTF (.glb) Worlds
//
// Drop this script into an `Assets/Editor/` folder in your Unity project (requires a
// glTF 2.0 importer such as UnityGLTF or glTFast that invokes OnPostprocessModel or
// preserves node hierarchy and extras).
//
// Features:
// - Inspects imported Educore `.glb` hierarchies (`world.glb` or assets containing
//   Educore object nodes).
// - Adds `BoxCollider` components to buildings, castles, walls, trees, water, props,
//   and ground geometry.
// - Configures trigger `BoxCollider` door components (`EducoreDoorTrigger`) at building
//   entrances and links them to interior root transforms if exported with
//   `single_scene_interiors`.
// - Marks `player` nodes as spawn points (`EducoreSpawnPoint`) and tags them with
//   `"Respawn"` when available.
// - Wires directional sun lighting on root scenes.

using System;
using UnityEngine;
#if UNITY_EDITOR
using UnityEditor;
#endif

/// <summary>
/// Runtime component attached to building door triggers imported from Educore.
/// </summary>
public class EducoreDoorTrigger : MonoBehaviour
{
    [Tooltip("Target interior root transform when exported with single_scene_interiors.")]
    public Transform targetInteriorRoot;

    [Tooltip("Index of the interior scene in the glTF file.")]
    public int interiorSceneIndex = -1;
}

/// <summary>
/// Runtime component attached to the player spawn point imported from Educore.
/// </summary>
public class EducoreSpawnPoint : MonoBehaviour
{
    public Vector2 gridPosition;
}

/// <summary>
/// Runtime metadata component attached to interactive Educore entities (NPCs, items, props).
/// </summary>
public class EducoreObjectMetadata : MonoBehaviour
{
    public string objectType;
    public string objectId;
    public int floors = 1;
}

#if UNITY_EDITOR
public class UnityEducoreImporter : AssetPostprocessor
{
    private const float FloorHeight = 2.0f;
    private static readonly Vector3 BuildingFootprint = new Vector3(8.0f, 2.0f, 8.0f);

    private void OnPostprocessModel(GameObject root)
    {
        if (root == null)
        {
            return;
        }

        if (!assetPath.EndsWith(".glb", StringComparison.OrdinalIgnoreCase) &&
            !assetPath.EndsWith(".gltf", StringComparison.OrdinalIgnoreCase))
        {
            return;
        }

        ConfigureSunLight(root);

        foreach (Transform child in root.transform)
        {
            ProcessNode(root.transform, child);
        }
    }

    private static void ConfigureSunLight(GameObject root)
    {
        Light existingLight = root.GetComponentInChildren<Light>();
        if (existingLight != null)
        {
            existingLight.type = LightType.Directional;
            existingLight.shadows = LightShadows.Soft;
            return;
        }

        GameObject sunObj = new GameObject("EducoreSunLight");
        sunObj.transform.SetParent(root.transform, false);
        sunObj.transform.rotation = Quaternion.Euler(50.0f, -30.0f, 0.0f);

        Light sun = sunObj.AddComponent<Light>();
        sun.type = LightType.Directional;
        sun.color = new Color(0.9f, 0.9f, 0.8f);
        sun.intensity = 1.0f;
        sun.shadows = LightShadows.Soft;
    }

    private static void ProcessNode(Transform sceneRoot, Transform node)
    {
        string nodeName = node.name.ToLowerInvariant();

        if (nodeName == "ground")
        {
            AddBoxCollider(node.gameObject, new Vector3(256.0f, 1.0f, 256.0f), new Vector3(0.0f, -0.5f, 0.0f));
            return;
        }

        if (nodeName.Contains("interior"))
        {
            foreach (Transform child in node)
            {
                ProcessNode(sceneRoot, child);
            }
            return;
        }

        EducoreObjectMetadata meta = node.GetComponent<EducoreObjectMetadata>();
        string objType = meta != null && !string.IsNullOrEmpty(meta.objectType)
            ? meta.objectType
            : InferObjectType(nodeName);

        switch (objType)
        {
            case "building":
            case "castle":
                ConfigureBuilding(sceneRoot, node, meta != null ? Mathf.Max(1, meta.floors) : EstimateFloors(node));
                break;

            case "wall":
                AddBoxCollider(node.gameObject, new Vector3(1.0f, 2.0f, 1.0f), new Vector3(0.5f, 1.0f, -0.5f));
                break;

            case "tree":
                AddBoxCollider(node.gameObject, new Vector3(0.6f, 2.5f, 0.6f), new Vector3(0.0f, 1.25f, 0.0f));
                break;

            case "water":
                AddBoxCollider(node.gameObject, new Vector3(1.0f, 0.5f, 1.0f), Vector3.zero);
                break;

            case "player":
                ConfigurePlayerSpawn(node);
                break;

            case "npc":
            case "item":
                ConfigureInteractiveTrigger(node, objType, meta != null ? meta.objectId : node.name);
                break;

            case "prop":
                AddFitBoxCollider(node.gameObject);
                break;
        }
    }

    private static string InferObjectType(string lowerName)
    {
        string[] knownTypes =
        {
            "building", "castle", "road", "player", "wall",
            "tree", "npc", "item", "animal", "water", "high_grass", "bridge"
        };

        foreach (string t in knownTypes)
        {
            if (lowerName == t || lowerName.StartsWith(t + " ") || lowerName.StartsWith(t + "_"))
            {
                return t;
            }
        }
        return "prop";
    }

    private static int EstimateFloors(Transform building)
    {
        Bounds bounds = ComputeLocalHierarchyBounds(building);
        if (bounds.size.y > 0.1f)
        {
            return Mathf.Max(1, Mathf.RoundToInt(bounds.size.y / FloorHeight));
        }
        return 1;
    }

    private static void ConfigureBuilding(Transform sceneRoot, Transform building, int floors)
    {
        float height = floors * FloorHeight;
        AddBoxCollider(
            building.gameObject,
            new Vector3(BuildingFootprint.x, height, BuildingFootprint.z),
            new Vector3(0.0f, height * 0.5f, 0.0f));

        GameObject doorObj = new GameObject("DoorTrigger");
        doorObj.transform.SetParent(building, false);
        doorObj.transform.localPosition = new Vector3(0.0f, 1.1f, 4.0f);

        BoxCollider trigger = doorObj.AddComponent<BoxCollider>();
        trigger.isTrigger = true;
        trigger.size = new Vector3(1.6f, 2.2f, 1.2f);

        EducoreDoorTrigger door = doorObj.AddComponent<EducoreDoorTrigger>();
        string expectedInteriorPrefix = string.Format(
            "{0} interior ({1}",
            building.name,
            Mathf.RoundToInt(building.localPosition.x));

        foreach (Transform sibling in sceneRoot)
        {
            if (sibling.name.StartsWith(expectedInteriorPrefix, StringComparison.OrdinalIgnoreCase))
            {
                door.targetInteriorRoot = sibling;
                break;
            }
        }
    }

    private static void ConfigurePlayerSpawn(Transform playerNode)
    {
        EducoreSpawnPoint spawn = playerNode.gameObject.AddComponent<EducoreSpawnPoint>();
        spawn.gridPosition = new Vector2(playerNode.localPosition.x, -playerNode.localPosition.z);
        playerNode.gameObject.tag = "Respawn";
    }

    private static void ConfigureInteractiveTrigger(Transform entityNode, string objType, string objId)
    {
        if (entityNode.GetComponent<EducoreObjectMetadata>() == null)
        {
            EducoreObjectMetadata meta = entityNode.gameObject.AddComponent<EducoreObjectMetadata>();
            meta.objectType = objType;
            meta.objectId = objId;
        }

        BoxCollider trigger = entityNode.gameObject.AddComponent<BoxCollider>();
        trigger.isTrigger = true;
        trigger.center = new Vector3(0.0f, 1.0f, 0.0f);
        trigger.size = new Vector3(1.5f, 2.0f, 1.5f);
    }

    private static void AddBoxCollider(GameObject target, Vector3 size, Vector3 center)
    {
        BoxCollider col = target.GetComponent<BoxCollider>();
        if (col == null)
        {
            col = target.AddComponent<BoxCollider>();
        }
        col.size = size;
        col.center = center;
    }

    private static void AddFitBoxCollider(GameObject target)
    {
        Bounds bounds = ComputeLocalHierarchyBounds(target.transform);
        if (bounds.size.sqrMagnitude < 0.001f)
        {
            bounds = new Bounds(new Vector3(0.0f, 0.5f, 0.0f), new Vector3(0.9f, 1.0f, 0.9f));
        }
        AddBoxCollider(target, bounds.size, bounds.center);
    }

    private static Bounds ComputeLocalHierarchyBounds(Transform root)
    {
        MeshFilter[] filters = root.GetComponentsInChildren<MeshFilter>();
        if (filters.Length == 0)
        {
            return new Bounds(Vector3.zero, Vector3.zero);
        }

        Bounds combined = new Bounds();
        bool initialized = false;
        Matrix4x4 worldToRoot = root.worldToLocalMatrix;

        foreach (MeshFilter mf in filters)
        {
            if (mf.sharedMesh == null)
            {
                continue;
            }
            Matrix4x4 toRoot = worldToRoot * mf.transform.localToWorldMatrix;
            Bounds meshBounds = mf.sharedMesh.bounds;
            Vector3 c = meshBounds.center;
            Vector3 e = meshBounds.extents;

            for (int ix = -1; ix <= 1; ix += 2)
            {
                for (int iy = -1; iy <= 1; iy += 2)
                {
                    for (int iz = -1; iz <= 1; iz += 2)
                    {
                        Vector3 corner = toRoot.MultiplyPoint3x4(
                            c + new Vector3(e.x * ix, e.y * iy, e.z * iz));
                        if (!initialized)
                        {
                            combined = new Bounds(corner, Vector3.zero);
                            initialized = true;
                        }
                        else
                        {
                            combined.Encapsulate(corner);
                        }
                    }
                }
            }
        }
        return combined;
    }
}
#endif
