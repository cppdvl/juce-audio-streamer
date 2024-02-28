#!/bin/bash


#Prompt for Apple ID

echo "Enter your Apple ID: "
read appleid

#Prompt for App Specific Password

echo "Enter your App Specific Password: "
read password

xcrun altool --list-providers -u $appleid -p $password

echo "Use the WWDRTeamID and type it here:"
read teamid



xcrun notarytool store-credentials --apple-id $appleid --password $password --team-id $teamid


