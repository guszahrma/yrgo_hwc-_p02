# Git kommandon


## Skapa ett lokalt repo
```Bash
git init --initial-branch=main
```


## Kolla  status
```Bash
git status
```

## Registrera din användare
```Bash
git config --global user.name "Martin Zahr"
git config --global user.email "gu92maak@gmail.com"
```




## Lägg till fil i Git

Vi lägger till filen readme.md
```Bash
git add readme.md
```

## Kolla git historiken
```Bash
git log
```
Avsluta med q


## Koppla repot till ett GitHub repo
Skapa först repot i [GitHub](https://github.com/guszahrma?tab=repositories) och sätt sedan rätt URL som origin

```Bash
git remote add origin git@github.com:guszahrma/yrgo_hwc-_p02.git
```

## Lägg till en SSH-nyckel

Generera en SSH-nyckel
```Bash
ssh-keygen
```

Klicka ENTER -> ENTER (Inga Passphrases osv)
ta innehållet i den nyskapade .pub filen och kopiera in i din [GitHub-profil](https://github.com/settings/keys)



```Bash
git push --set-upstream origin main
```
