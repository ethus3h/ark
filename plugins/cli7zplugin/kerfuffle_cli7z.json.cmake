{
    "KPlugin": {
        "Description": "Full support for the zip and 7z archive formats", 
        "Description[ca@valencia]": "Implementació completa dels formats d'arxiu «zip» i «7z»", 
        "Description[ca]": "Implementació completa dels formats d'arxiu «zip» i «7z»", 
        "Description[es]": "Uso total de los formatos de archivos comprimidos zip y 7z", 
        "Description[it]": "Supporto completo per i formati di archivi zip e 7z", 
        "Description[nl]": "Volledige ondersteuning voor de zip- en 7z-archiefformaten", 
        "Description[pl]": "Pełna obsługa dla formatów archiwów zip oraz 7z", 
        "Description[pt]": "Suporte total para os formatos de pacotes ZIP e 7z", 
        "Description[sk]": "Plná podpora pre archívne formáty zip a 7z", 
        "Description[sr@ijekavian]": "Пуна подршка за архивске формате ЗИП и 7зип", 
        "Description[sr@ijekavianlatin]": "Puna podrška za arhivske formate ZIP i 7zip", 
        "Description[sr@latin]": "Puna podrška za arhivske formate ZIP i 7zip", 
        "Description[sr]": "Пуна подршка за архивске формате ЗИП и 7зип", 
        "Description[sv]": "Fullt stöd för zip- och 7z-arkivformaten", 
        "Description[uk]": "Повноцінна підтримка архівів у форматах zip і 7z", 
        "Description[x-test]": "xxFull support for the zip and 7z archive formatsxx", 
        "Id": "kerfuffle_cli7z", 
        "MimeTypes": [
            "@SUPPORTED_MIMETYPES@"
        ], 
        "Name": "P7zip plugin", 
        "Name[ca@valencia]": "Connector del P7zip", 
        "Name[ca]": "Connector del P7zip", 
        "Name[es]": "Complemento P7zip", 
        "Name[it]": "Estensione P7zip", 
        "Name[nl]": "P7zip-plug-in", 
        "Name[pl]": "Wtyczka p7zip", 
        "Name[pt]": "'Plugin' do P7zip", 
        "Name[sk]": "Plugin P7zip", 
        "Name[sr@ijekavian]": "Прикључак за П7зип", 
        "Name[sr@ijekavianlatin]": "Priključak za p7zip", 
        "Name[sr@latin]": "Priključak za p7zip", 
        "Name[sr]": "Прикључак за П7зип", 
        "Name[sv]": "P7zip-insticksprogram", 
        "Name[uk]": "Додаток P7zip", 
        "Name[x-test]": "xxP7zip pluginxx", 
        "ServiceTypes": [
            "Kerfuffle/Plugin"
        ], 
        "Version": "@KDE_APPLICATIONS_VERSION@"
    }, 
    "X-KDE-Kerfuffle-ReadOnlyExecutables": [
        "7z"
    ], 
    "X-KDE-Kerfuffle-ReadWrite": true, 
    "X-KDE-Kerfuffle-ReadWriteExecutables": [
        "7z"
    ], 
    "X-KDE-Priority": 180, 
    "application/x-7z-compressed": {
        "CompressionLevelDefault": 5, 
        "CompressionLevelMax": 9, 
        "CompressionLevelMin": 0, 
        "CompressionMethodDefault": "LZMA2", 
        "CompressionMethods": {
            "BZip2": "BZip2", 
            "Copy": "Copy", 
            "Deflate": "Deflate", 
            "LZMA": "LZMA", 
            "LZMA2": "LZMA2", 
            "PPMd": "PPMd"
        }, 
        "EncryptionMethodDefault": "AES256", 
        "EncryptionMethods": [
            "AES256"
        ], 
        "HeaderEncryption": true, 
        "SupportsMultiVolume": true, 
        "SupportsTesting": true
    }, 
    "application/zip": {
        "CompressionLevelDefault": 5, 
        "CompressionLevelMax": 9, 
        "CompressionLevelMin": 0, 
        "CompressionMethodDefault": "Deflate", 
        "CompressionMethods": {
            "BZip2": "BZip2", 
            "Copy": "Copy", 
            "Deflate": "Deflate", 
            "Deflate64": "Deflate64", 
            "LZMA": "LZMA", 
            "PPMd": "PPMd"
        }, 
        "Encryption": true, 
        "EncryptionMethodDefault": "AES256", 
        "EncryptionMethods": [
            "AES256", 
            "AES192", 
            "AES128", 
            "ZipCrypto"
        ], 
        "SupportsMultiVolume": true, 
        "SupportsTesting": true
    }
}
