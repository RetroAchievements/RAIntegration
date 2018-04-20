Function DeployIntegration
{
    Param(
        [Parameter(Mandatory)]
        [ValidateSet("True","False")]
        $ForceUpdate
        )

    Process
    {
        ################################################################################
        # Custom Variables:

        $IntegrationDLLSource = "RA_Integration/RA_Integration.dll"

        $VersionDoc = "..\RAWeb\public\LatestIntegration.html"

        $ExpectedTag = "RAIntegration"


        ################################################################################
        # Global Variables:

        $Password = ConvertTo-SecureString 'Unused' -AsPlainText -Force
        $Credential = New-Object System.Management.Automation.PSCredential ('ec2-user', $Password)
        $KeyPath = ".\RetroAchievementsKey"
        $TargetURL = "www.RetroAchievements.org"
        $WebRoot = "/var/www/html/public"
        $WebRootBin = "/var/www/html/public/bin"


        ################################################################################
        # Test we are ready for release
        $latestTag = git describe --tags --match "$($ExpectedTag).*"
        $diffs = git diff HEAD
        if( ![string]::IsNullOrEmpty( $diffs ) )
        {
            if( $ForceUpdate -ne "True" )
            {
                throw "Changes exist, cannot deploy!"
            }
            else
            {
                Write-Warning "Changes exist, deploying anyway!"
            }
        }

        $newHTMLContent = "0." + $latestTag.Substring( $ExpectedTag.Length + 1, 3 )
        $currentVersion = Get-Content $VersionDoc
        
        if( $newHTMLContent -eq $currentVersion )
        {
            if( $ForceUpdate -ne "True" )
            {
                throw "We are already on version $currentVersion, nothing to do!"
            }
            else
            {
                Write-Warning "We are already on version $currentVersion, deploying anyway!"
            }
        }

        Set-Content $VersionDoc $newHTMLContent


        ################################################################################
        # Upload

        # Establish the SFTP connection
        $session = ( New-SFTPSession -ComputerName $TargetURL -Credential $Credential -KeyFile $KeyPath )

        # Upload the new version html
        Set-SFTPFile -SessionId $session.SessionId -LocalFile $VersionDoc -RemotePath $WebRoot -Overwrite

        # Upload the zip to the SFTP bin path
        Set-SFTPFile -SessionId $session.SessionId -LocalFile $IntegrationDLLSource -RemotePath $WebRootBin -Overwrite

        # Disconnect SFTP session
        $session.Disconnect()

        Write-Warning "Published $latestTag"
    }
}


################################################################################
# Set working directory:

$dir = Split-Path $MyInvocation.MyCommand.Path
Set-Location $dir

DeployIntegration