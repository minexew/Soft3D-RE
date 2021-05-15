using System;
using System.Reflection;


namespace BspViewer.Utils
{
    class ReflectionUtils
    {
        public static object CallToClass(string name, string methodName, object[] parameters)
        {
            Type t = Type.GetType(name);
            if (t == null)
                throw new MissingMemberException("Class " + name + " not found.");

            object instance = Activator.CreateInstance(t);
            MethodInfo method = t.GetMethod(methodName);

            if(method != null)
                method.Invoke(instance, parameters);

            return instance;
        }
    }
}
