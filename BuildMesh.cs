using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.IO;

public class BuildMesh : MonoBehaviour
{
	[SerializeField] GameObject _GameObject;
	[SerializeField] Texture2D _Texture;

	void Start()
	{
		StreamWriter writer = new StreamWriter(Path.Combine(Application.dataPath, "Mesh.h"));
		MeshFilter meshFilter = _GameObject.GetComponent<MeshFilter>();
		Vector3[] vertices = meshFilter.sharedMesh.vertices;
		Vector2[] uvs = meshFilter.sharedMesh.uv;
		int[] triangles = meshFilter.sharedMesh.triangles;
		int count = triangles.Length;
		writer.WriteLine("typedef struct float4 {float x; float y; float z; float w;} float4;");
		writer.WriteLine("");
		writer.WriteLine("float4 vertices["+count.ToString()+"] = {");
		for (int i = 0; i < count; i++)
		{
			Vector3 vertex = _GameObject.transform.TransformPoint(vertices[triangles[i]]);
			string temp = "{";
			temp += vertex.x.ToString("0.0000", System.Globalization.CultureInfo.InvariantCulture);
			temp += "f, ";
			temp += vertex.y.ToString("0.0000", System.Globalization.CultureInfo.InvariantCulture);
			temp += "f, ";
			temp += vertex.z.ToString("0.0000", System.Globalization.CultureInfo.InvariantCulture);
			temp += "f, 1.0f},";
			writer.WriteLine(temp);
		}
		writer.WriteLine("};");
		writer.WriteLine();
		writer.WriteLine("typedef struct float2 {float x; float y;} float2;");
		writer.WriteLine("");
		writer.WriteLine("float2 uvs["+count.ToString()+"] = {");
		for (int i = 0; i < count; i++)
		{
			Vector2 uv = uvs[triangles[i]];
			string temp = "{";
			temp += uv.x.ToString("0.0000", System.Globalization.CultureInfo.InvariantCulture);
			temp += "f, ";
			temp += uv.y.ToString("0.0000", System.Globalization.CultureInfo.InvariantCulture);
			temp += "f},";
			writer.WriteLine(temp);
		}
		writer.WriteLine("};");
		writer.WriteLine();	
		Color[] colors = _Texture.GetPixels(0);
		writer.WriteLine("int texture["+colors.Length.ToString()+"] = {");
		string line = "";
		for (int i = 0; i < colors.Length; i++)
		{
			byte[] bytes = new byte[4];
			bytes[0] = System.Convert.ToByte(colors[i].b * 255.0f);
			bytes[1] = System.Convert.ToByte(colors[i].g * 255.0f);
			bytes[2] = System.Convert.ToByte(colors[i].r * 255.0f);
			bytes[3] = System.Convert.ToByte(colors[i].a * 255.0f);
			int number = System.BitConverter.ToInt32(bytes, 0);
			line = line + number.ToString() +", ";
			if ((i % 16 == 0) && (i > 0))
			{
				writer.WriteLine(line);
				line = "";
			}
		}
		writer.WriteLine("};");
		writer.Close();
	}
}