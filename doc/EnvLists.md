# Environment List

## UnrealZoo_UE5_5 (100+ maps)

⚠️ **Note**: The UE5.5 full version package contains **100+ scenes**, but currently only a subset have configuration files ready to use. More configuration files will be added in future updates.

### Available Maps with Configuration Files

The following maps have ready-to-use configuration files in `gym_unrealcv/envs/setting/Track/`:

| Map Name              | Description                    | Features                          |
|-----------------------|--------------------------------|-----------------------------------|
| DowntownWest          | Downtown West (City Scene)     | Human, Car, Motorcycle, Drone     |
| ModularCity           | Modular City                   | Human, Car, Drone                 |
| Old_Town              | Modular Old Town               | Human, Historical Architecture    |
| Map_ChemicalPlant_1   | Chemical Factory               | Industrial Environment            |
| MiddleEast            | Middle East Scene              | Desert Architecture               |
| Demo_Roof             | Roof-City Scene                | Urban Rooftop Environment         |
| FlexibleRoom          | Flexible Room                  | Indoor Environment                |

### Creating Configuration Files for Other Maps

To use other maps from the 100+ scenes available in the full package, you can generate configuration files using:

```bash
python generate_env_config.py --env-map <MapName>
```

For more details, see the [Configuration File Guide](ConfigFileGuide.md).

### Complete Scene Gallery

For a complete list and preview of all 100+ scenes, visit the **[UnrealZoo Scene Gallery](https://www.notion.so/Scene-Gallery-a801475ff98943159da66f641f4c38b2)**.
