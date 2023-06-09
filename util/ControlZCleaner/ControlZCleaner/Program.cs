if( args.Length != 2)
{
    Console.WriteLine("Usage: ControlZCleaner ROOT_PATH FILE_SPEC"  );
    Console.WriteLine("Example: ControlZCleaner c:\\user *.aza");
    return;
}

var path = args[0];
var spec = args[1];
foreach (var item in Directory.EnumerateFiles(path, spec, SearchOption.AllDirectories))
{
    Console.WriteLine($"Working {item}..."  );
    doFile(item);
}
Console.WriteLine("All Done");

void doFile(string src)
{
    var text = File.ReadAllText(src, System.Text.Encoding.GetEncoding("ISO-8859-1"));
    int index = text.IndexOf((char)0x1a);
    if (index < 0) return;
    var bakName = Path.ChangeExtension(src, ".old");
    File.Delete(bakName);
    File.Move(src,bakName);
    File.WriteAllText(src,text.Substring(0,index), System.Text.Encoding.GetEncoding("ISO-8859-1"));
}
