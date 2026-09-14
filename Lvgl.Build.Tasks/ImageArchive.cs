using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace Mile.Project.Helpers
{
    public class ImageArchive
    {
        private const int StartSize = 8;
        private const string Start = "!<arch>\n";
        private const string End = "`\n";
        private const string Pad = "\n";
        private const string LinkerMember = "/               ";
        private const string LongnamesMember = "//              ";
        private const string HybridmapMember = "/<HYBRIDMAP>/   ";
        private const string EcSymbolsMember = "/<ECSYMBOLS>/   ";

        /// <summary>
        /// Each member (linker, longnames, or object-file member) is preceded
        /// by a header. An archive member header has the following format, in
        /// which each field is an ASCII text string that is left justified and
        /// padded with spaces to the end of the field. There is no terminating
        /// null character in any of these fields. Each member header starts on
        /// the first even address after the end of the previous archive member,
        /// one byte '\n' (PAD) may be inserted after an archive
        /// member to make the following member start on an even address.
        /// </summary>
        /// <see cref="https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#archive-member-headers"/>
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
        private struct MemberHeader
        {
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
            private byte[] RawName;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 12)]
            private byte[] RawDate;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 6)]
            private byte[] RawUserID;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 6)]
            private byte[] RawGroupID;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
            private byte[] RawMode;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 10)]
            private byte[] RawSize;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
            private byte[] RawEndHeader;

            /// <summary>
            /// The name of the archive member, with a slash (/) appended to
            /// terminate the name. If the first character is a slash, the name
            /// has a special interpretation, as described in the following
            /// table.
            /// </summary>
            public string Name
            {
                get
                {
                    return Encoding.ASCII.GetString(RawName);
                }
            }

            /// <summary>
            /// The date and time that the archive member was created: This is
            /// the ASCII decimal representation of the number of seconds since
            /// 1/1/1970 UCT.
            /// </summary>
            public string Date
            {
                get
                {
                    return Encoding.ASCII.GetString(RawDate);
                }
            }

            /// <summary>
            /// An ASCII decimal representation of the user ID. This field does
            /// not contain a meaningful value on Windows platforms because
            /// Microsoft tools emit all blanks.
            /// </summary>
            public string UserID
            {
                get
                {
                    return Encoding.ASCII.GetString(RawUserID);
                }
            }

            /// <summary>
            /// An ASCII decimal representation of the group ID. This field does
            /// not contain a meaningful value on Windows platforms because
            /// Microsoft tools emit all blanks.
            /// </summary>
            public string GroupID
            {
                get
                {
                    return Encoding.ASCII.GetString(RawGroupID);
                }
            }

            /// <summary>
            /// An ASCII octal representation of the member's file mode. This is
            /// the ST_MODE value from the C run-time function _wstat.
            /// </summary>
            public string Mode
            {
                get
                {
                    return Encoding.ASCII.GetString(RawMode);
                }
            }

            /// <summary>
            /// An ASCII decimal representation of the total size of the archive
            /// member, not including the size of the header.
            /// </summary>
            public string Size
            {
                get
                {
                    return Encoding.ASCII.GetString(RawSize);
                }
            }

            /// <summary>
            /// The two bytes (0x60 0x0A) in the C string "`\n"
            /// (<see cref="End"/>).
            /// </summary>
            public string EndHeader
            {
                get
                {
                    return Encoding.ASCII.GetString(RawEndHeader);
                }
            }
        }

        private const int MemberHeaderSize = 60;

        private static T BytesToStructure<T>(byte[] Bytes)
        {
            GCHandle Handle = GCHandle.Alloc(Bytes, GCHandleType.Pinned);
            T Result = Marshal.PtrToStructure<T>(Handle.AddrOfPinnedObject());
            Handle.Free();
            return Result;
        }

        public struct Member
        {
            public string Name;
            public int Offset;
            public ReadOnlyMemory<byte> Content;
        }

        public struct Archive
        {
            public List<Member> Members;
            public int FirstLinkerIndex;
            public int SecondLinkerIndex;
            public int LongnamesIndex;
            public int EcSymbolsIndex;
            public List<(string Key, string Value)> Symbols;
            public List<(string Key, string Value)> EcSymbols;
        }

        public static Archive Parse(
            ReadOnlyMemory<byte> Content)
        {
            int Offset = 0;

            if (Start != Encoding.ASCII.GetString(
                Content.Slice(Offset, StartSize).Span.ToArray()))
            {
                throw new ArgumentException("Invalid image archive file.");
            }
            Offset += StartSize;

            Archive Result;

            Result.Members = new List<Member>();
            while (Offset < Content.Length)
            {
                MemberHeader Header = BytesToStructure<MemberHeader>(
                    Content.Slice(Offset, MemberHeaderSize).ToArray());
                if (End != Header.EndHeader)
                {
                    break;
                }
                Member Current;
                Current.Name = Header.Name;
                Current.Offset = Offset;
                Offset += MemberHeaderSize;
                Current.Content = Content.Slice(
                    Offset,
                    Convert.ToInt32(Header.Size));
                Result.Members.Add(Current);
                Offset += Convert.ToInt32(Header.Size);
                // Align the offset to the next even address.
                Offset = (Offset + 1) & ~1;
            }
            Result.FirstLinkerIndex = Result.Members.FindIndex(
                Member => Member.Name == LinkerMember);
            Result.SecondLinkerIndex = Result.Members.FindIndex(
                Result.FirstLinkerIndex + 1,
                Member => Member.Name == LinkerMember);
            Result.LongnamesIndex = Result.Members.FindIndex(
                Member => Member.Name == LongnamesMember);
            Result.EcSymbolsIndex = Result.Members.FindIndex(
                Member => Member.Name == EcSymbolsMember);
            Result.Symbols =
                (-1 != Result.FirstLinkerIndex)
                ? new List<(string Key, string Value)>()
                : null;
            Result.EcSymbols =
                (-1 != Result.EcSymbolsIndex)
                ? new List<(string Key, string Value)>()
                : null;

            for (int i = 0; i < Result.Members.Count; ++i)
            {
                Member Current = Result.Members[i];

                int SplitterPosition = Current.Name.IndexOf('/');
                if (-1 == SplitterPosition)
                {
                    continue;
                }

                try
                {
                    if (0 == SplitterPosition)
                    {
                        int NameOffset = Convert.ToInt32(
                            Result.Members[i].Name.Substring(1));

                        Current.Name = Encoding.ASCII.GetString(
                            Result.Members[Result.LongnamesIndex].Content.Slice(
                                NameOffset).ToArray()).Split('\0')[0];
                    }
                    else
                    {
                        Current.Name = Result.Members[i].Name.Substring(
                            0,
                            SplitterPosition);
                    }
                    Result.Members[i] = Current;
                }
                catch
                {

                }
            }

            if (null != Result.Symbols)
            {
                int Current = 0;

                int SymbolsCount = 0;
                {
                    byte[] RawBytes =
                        Result.Members[Result.FirstLinkerIndex].Content.Slice(
                            Current,
                            sizeof(int)).ToArray();
                    if (BitConverter.IsLittleEndian)
                    {
                        Array.Reverse(RawBytes);
                    }
                    SymbolsCount = BitConverter.ToInt32(RawBytes, 0);
                    Current += sizeof(int);
                }

                for (int i = 0; i < SymbolsCount; ++i)
                {
                    int MemberOffset = 0;
                    {
                        byte[] RawBytes =
                        Result.Members[Result.FirstLinkerIndex].Content.Slice(
                            Current,
                            sizeof(int)).ToArray();
                        if (BitConverter.IsLittleEndian)
                        {
                            Array.Reverse(RawBytes);
                        }
                        MemberOffset = BitConverter.ToInt32(RawBytes, 0);
                        Current += sizeof(int);
                    }

                    int MemberIndex = Result.Members.FindIndex(
                        Member => Member.Offset == MemberOffset);
                    if (-1 == MemberIndex)
                    {
                        break;
                    }

                    (string Key, string Value) Symbol = (string.Empty, string.Empty);
                    Symbol.Value = Result.Members[MemberIndex].Name;
                    Result.Symbols.Add(Symbol);
                }

                string[] RawStrings = Encoding.ASCII.GetString(
                    Result.Members[Result.FirstLinkerIndex].Content.Slice(
                        Current).ToArray()).Split('\0');

                for (int i = 0; i < SymbolsCount; ++i)
                {
                    (string Key, string Value) Symbol = Result.Symbols[i];
                    Symbol.Key = RawStrings[i];
                    Result.Symbols[i] = Symbol;
                }
            }

            if (null != Result.EcSymbols)
            {
                int Current = 0;

                int SymbolsCount = 0;
                {
                    byte[] RawBytes =
                        Result.Members[Result.EcSymbolsIndex].Content.Slice(
                            Current,
                            sizeof(int)).ToArray();
                    if (!BitConverter.IsLittleEndian)
                    {
                        Array.Reverse(RawBytes);
                    }
                    SymbolsCount = BitConverter.ToInt32(RawBytes, 0);
                    Current += sizeof(int);
                }

                for (int i = 0; i < SymbolsCount; ++i)
                {
                    int EcSymbolOffset = Result.EcSymbolsIndex;
                    {
                        byte[] RawBytes =
                            Result.Members[Result.EcSymbolsIndex].Content.Slice(
                                Current,
                                sizeof(ushort)).ToArray();
                        if (!BitConverter.IsLittleEndian)
                        {
                            Array.Reverse(RawBytes);
                        }
                        EcSymbolOffset += BitConverter.ToUInt16(RawBytes, 0);
                        Current += sizeof(ushort);
                    }

                    (string Key, string Value) EcSymbol = (string.Empty, string.Empty);
                    EcSymbol.Value = Result.Members[EcSymbolOffset].Name;
                    Result.EcSymbols.Add(EcSymbol);
                }

                string[] RawStrings = Encoding.ASCII.GetString(
                    Result.Members[Result.EcSymbolsIndex].Content.Slice(
                        Current).ToArray()).Split('\0');

                for (int i = 0; i < SymbolsCount; ++i)
                {
                    (string Key, string Value) EcSymbol = Result.EcSymbols[i];
                    EcSymbol.Key = RawStrings[i];
                    Result.EcSymbols[i] = EcSymbol;
                }
            }

            return Result;
        }

        public static Archive Parse(
            string FilePath)
        {
            return Parse(File.ReadAllBytes(FilePath));
        }

        public static SortedDictionary<string, SortedSet<string>> CategorizeSymbols(
            List<(string Key, string Value)> Symbols,
            bool IsIntelArchitecture32 = false)
        {
            SortedDictionary<string, SortedSet<string>> Result =
                new SortedDictionary<string, SortedSet<string>>();
            foreach (var Symbol in Symbols)
            {
                if (!Result.ContainsKey(Symbol.Value))
                {
                    Result.Add(Symbol.Value, new SortedSet<string>());
                }

                if (Symbol.Key[0] == 0x7f ||
                    Symbol.Key == "__NULL_IMPORT_DESCRIPTOR" ||
                    Symbol.Key.StartsWith("__IMPORT_DESCRIPTOR_"))
                {
                    continue;
                }

                const string SharpSeparator = "#";
                const string ImpAuxSeparator = "__imp_aux_";
                const string ImpSeparator = "__imp_";
                const string AuxImpCopySeparator = "__auximpcopy_";
                const string ImpChkSeparator = "__impchk_";

                if (Symbol.Key.StartsWith(SharpSeparator))
                {
                    Result[Symbol.Value].Add(
                        Symbol.Key.Substring(SharpSeparator.Length));
                }
                else
                {
                    int TrimIndex = -1;
                    if (Symbol.Key.StartsWith(ImpAuxSeparator))
                    {
                        TrimIndex = ImpAuxSeparator.Length;
                    }
                    else if (Symbol.Key.StartsWith(ImpSeparator))
                    {
                        TrimIndex = ImpSeparator.Length;
                    }
                    else if (Symbol.Key.StartsWith(AuxImpCopySeparator))
                    {
                        TrimIndex = AuxImpCopySeparator.Length;
                    }
                    else if (Symbol.Key.StartsWith(ImpChkSeparator))
                    {
                        TrimIndex = ImpChkSeparator.Length;
                    }

                    string TrimmedName = Symbol.Key;
                    if (TrimIndex > 0)
                    {
                        TrimmedName = TrimmedName.Substring(TrimIndex);
                        TrimmedName = string.Format(
                            "{0}{1}",
                            TrimmedName,
                            -1 != Symbols.FindIndex(
                                Symbol => Symbol.Key == TrimmedName)
                            ? string.Empty
                            : " DATA");
                    }
                    if (IsIntelArchitecture32)
                    {
                        if (TrimmedName.StartsWith("_"))
                        {
                            TrimmedName = TrimmedName.Substring(1);
                        }
                    }
                    Result[Symbol.Value].Add(TrimmedName);
                }
            }

            return Result;
        }

        public static SortedSet<string> ListSymbols(
            SortedDictionary<string, SortedSet<string>> Categories)
        {
            SortedSet<string> Result = new SortedSet<string>();
            foreach (var Category in Categories)
            {
                foreach (var Item in Category.Value)
                {
                    Result.Add(Item);
                }
            }
            return Result;
        }

        public static SortedSet<string> ListSymbols(
            List<(string Key, string Value)> Symbols,
            bool IsIntelArchitecture32 = false)
        {
            SortedDictionary<string, SortedSet<string>> SymbolCategories =
                CategorizeSymbols(Symbols, IsIntelArchitecture32);
            return ListSymbols(SymbolCategories);
        }
    }
}
